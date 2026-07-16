import bpy
import json
import os
import mathutils

bl_info = {
    "name": "CG2 Spline Exporter",
    "author": "CG2 Engine Tools",
    "version": (1, 0),
    "blender": (3, 0, 0),
    "location": "File > Export",
    "description": "Export Curve objects to CG2 splines.json",
    "category": "Import-Export",
}

class ExportCG2Splines(bpy.types.Operator):
    """Export Curves to splines.json"""
    bl_idname = "export_scene.cg2_splines"
    bl_label = "Export CG2 Splines"
    
    filepath: bpy.props.StringProperty(subtype="FILE_PATH")
    
    # 座標系の変換フラグ（BlenderはZ-up, ゲーム側はY-upを想定）
    convert_y_up: bpy.props.BoolProperty(
        name="Convert to Y-Up",
        description="Convert Blender Z-up to Engine Y-up (x, y, z -> x, z, y)",
        default=True,
    )

    def execute(self, context):
        rails = []
        
        # すべてのカーブオブジェクトを取得
        for obj in bpy.context.scene.objects:
            if obj.type == 'CURVE':
                # カーブをメッシュ化して頂点を取得するのが最も確実（モディファイア等も適用される）
                depsgraph = context.evaluated_depsgraph_get()
                eval_obj = obj.evaluated_get(depsgraph)
                mesh = eval_obj.to_mesh()
                
                points = []
                for v in mesh.vertices:
                    # ワールド座標に変換
                    world_coord = obj.matrix_world @ v.co
                    
                    x, y, z = world_coord.x, world_coord.y, world_coord.z
                    
                    if self.convert_y_up:
                        # Blender (X, Y, Z) -> Engine (X, Z, Y) 
                        points.append([round(x, 3), round(z, 3), round(y, 3)])
                    else:
                        points.append([round(x, 3), round(y, 3), round(z, 3)])
                
                eval_obj.to_mesh_clear()
                
                # 名前でローカル・ワールドを判別するか、単純に全部エクスポートする
                rails.append({
                    "name": obj.name,
                    "points": points
                })

        # JSON書き出し
        output_data = {"rails": rails}
        
        with open(self.filepath, 'w', encoding='utf-8') as f:
            json.dump(output_data, f, indent=2)
            
        self.report({'INFO'}, f"Exported {len(rails)} rails to {self.filepath}")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


def menu_func_export(self, context):
    self.layout.operator(ExportCG2Splines.bl_idname, text="CG2 Splines (.json)")

def register():
    bpy.utils.register_class(ExportCG2Splines)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)

def unregister():
    bpy.utils.unregister_class(ExportCG2Splines)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)

if __name__ == "__main__":
    register()
