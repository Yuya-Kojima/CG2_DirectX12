import bpy
import json
import os
import mathutils

bl_info = {
    "name": "CG2 レール出力ツール",
    "author": "CG2 Engine Tools",
    "version": (2, 3),
    "blender": (3, 0, 0),
    "location": "View3D > Sidebar > CG2",
    "description": "Export Curve/Path objects to CG2 JSON formats with resampling.",
    "category": "Import-Export",
}

def convert_coord(co, matrix_world):
    world_coord = matrix_world @ mathutils.Vector(co)
    # Z-up (Blender) から Y-up (Engine) への変換
    return [round(world_coord.x, 3), round(world_coord.z, 3), round(world_coord.y, 3)]

def resample_points(raw_points, interval_length):
    if len(raw_points) < 2 or interval_length <= 0.0:
        return raw_points

    # 各線分の長さを計算
    total_length = 0.0
    segments = []
    for i in range(1, len(raw_points)):
        p0 = mathutils.Vector(raw_points[i-1])
        p1 = mathutils.Vector(raw_points[i])
        dist = (p1 - p0).length
        segments.append({'p0': p0, 'p1': p1, 'dist': dist, 'accum_start': total_length})
        total_length += dist

    sampled_points = []
    current_target_dist = 0.0
    
    # 指定距離ごとに点をサンプリング（Lerp補間）
    for seg in segments:
        while current_target_dist >= seg['accum_start'] and current_target_dist <= seg['accum_start'] + seg['dist']:
            local_dist = current_target_dist - seg['accum_start']
            t = local_dist / seg['dist'] if seg['dist'] > 0 else 0
            new_p = seg['p0'].lerp(seg['p1'], t)
            sampled_points.append((new_p.x, new_p.y, new_p.z))
            current_target_dist += interval_length

    # 終点が少し離れていれば確実に追加
    last_p = raw_points[-1]
    if sampled_points:
        dist_to_last = (mathutils.Vector(sampled_points[-1]) - mathutils.Vector(last_p)).length
        if dist_to_last > interval_length * 0.1:
            sampled_points.append(last_p)
    else:
        sampled_points.append(last_p)
        
    return sampled_points

def export_splines_data(context):
    scene = context.scene
    splines_path = bpy.path.abspath(scene.cg2_spline_export_path)
    level_path = bpy.path.abspath(scene.cg2_level_export_path)
    
    do_resample = scene.cg2_enable_resampling
    sample_dist = scene.cg2_sample_distance
    
    enemy_rails = []
    camera_waypoints = []
    
    # モディファイアや解像度が適用された状態（評価済み）のデータを取得するため
    depsgraph = context.evaluated_depsgraph_get()

    for obj in scene.objects:
        if obj.type == 'CURVE':
            # 名前に「camera」が含まれていたらカメラレールとみなす
            is_camera = ("camera" in obj.name.lower())
            
            raw_points = []
            
            if do_resample and not is_camera:
                # 敵レールのみ：メッシュに変換して等間隔サンプリング用の頂点群を取得
                eval_obj = obj.evaluated_get(depsgraph)
                try:
                    mesh = bpy.data.meshes.new_from_object(eval_obj)
                    for v in mesh.vertices:
                        raw_points.append(v.co.copy())
                    bpy.data.meshes.remove(mesh)
                except Exception as e:
                    print(f"Failed to convert curve to mesh: {e}")
                    pass
            else:
                # カメラレールは常に制御点のみ抽出（エンジン側でCatmullRom補間するため）
                for spline in obj.data.splines:
                    if spline.type in ('NURBS', 'POLY'):
                        for p in spline.points:
                            raw_points.append(p.co.xyz.copy())
                    elif spline.type == 'BEZIER':
                        for bp in spline.bezier_points:
                            raw_points.append(bp.co.copy())

            if not raw_points:
                continue

            # サンプリング実行（敵レールのみ、カメラレールはスキップ）
            if do_resample and not is_camera:
                final_local_points = resample_points(raw_points, sample_dist)
            else:
                final_local_points = raw_points

            # Z-up -> Y-up 変換とワールド座標変換
            final_world_points = []
            for p in final_local_points:
                final_world_points.append(convert_coord(p, obj.matrix_world))
                
            if is_camera:
                if not camera_waypoints: # カメラレールは1本のみ想定
                    camera_waypoints = final_world_points
            else:
                if len(final_world_points) >= 2:
                    enemy_rails.append({"name": obj.name, "points": final_world_points})

    messages = []
    success_count = 0

    # 敵レールを splines.json に保存
    if enemy_rails:
        os.makedirs(os.path.dirname(splines_path), exist_ok=True)
        with open(splines_path, 'w', encoding='utf-8') as f:
            json.dump({"rails": enemy_rails}, f, indent=2)
        messages.append(f"Exported {len(enemy_rails)} enemy rails.")
        success_count += 1
        
    # カメラレールを camera_rail.json に保存（level_editor.json には一切触れない）
    if camera_waypoints and level_path.endswith(".json"):
        os.makedirs(os.path.dirname(level_path), exist_ok=True)
        with open(level_path, 'w', encoding='utf-8') as f:
            json.dump({"waypoints": camera_waypoints}, f, indent=4)
        messages.append("Exported camera waypoints.")
        success_count += 1
        
    if success_count == 0:
        return False, "No valid curves found to export."
        
    return True, " ".join(messages)

class CG2_OT_export_splines_quick(bpy.types.Operator):
    bl_idname = "cg2.export_splines_quick"
    bl_label = "データを出力する"

    def execute(self, context):
        success, msg = export_splines_data(context)
        if success:
            self.report({'INFO'}, msg)
            return {'FINISHED'}
        else:
            self.report({'WARNING'}, msg)
            return {'CANCELLED'}

class CG2_PT_spline_exporter(bpy.types.Panel):
    bl_label = "CG2 レール出力ツール"
    bl_idname = "CG2_PT_spline_exporter"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'CG2'

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        layout.label(text="敵のレール出力先:")
        layout.prop(scene, "cg2_spline_export_path", text="")
        
        layout.label(text="カメラレール出力先:")
        layout.prop(scene, "cg2_level_export_path", text="")
        
        layout.separator()
        layout.prop(scene, "cg2_enable_resampling", text="等速化を有効にする")
        if scene.cg2_enable_resampling:
            layout.prop(scene, "cg2_sample_distance", text="点の配置間隔 (m)")
            
        layout.separator()
        layout.operator("cg2.export_splines_quick", text="データを出力する", icon='EXPORT')

classes = (
    CG2_OT_export_splines_quick,
    CG2_PT_spline_exporter,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
        
    default_splines = r"C:\Users\K024G\source\repos\CG2\project\Application\resources\levels\splines.json"
    default_level = r"C:\Users\K024G\source\repos\CG2\project\Application\resources\levels\camera_rail.json"
    
    bpy.types.Scene.cg2_spline_export_path = bpy.props.StringProperty(
        name="Splines Path", subtype='FILE_PATH', default=default_splines
    )
    bpy.types.Scene.cg2_level_export_path = bpy.props.StringProperty(
        name="Level Path", subtype='FILE_PATH', default=default_level
    )
    bpy.types.Scene.cg2_enable_resampling = bpy.props.BoolProperty(
        name="Enable Resampling",
        description="Resample curve to have points at equal distances",
        default=True
    )
    bpy.types.Scene.cg2_sample_distance = bpy.props.FloatProperty(
        name="Sample Distance",
        description="Distance between points in meters",
        default=0.5,
        min=0.01,
        max=10.0
    )

def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
        
    del bpy.types.Scene.cg2_spline_export_path
    del bpy.types.Scene.cg2_level_export_path
    del bpy.types.Scene.cg2_enable_resampling
    del bpy.types.Scene.cg2_sample_distance

if __name__ == "__main__":
    register()
