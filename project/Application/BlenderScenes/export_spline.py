import bpy
import json
import os
import mathutils

bl_info = {
    "name": "CG2 Spline Exporter",
    "author": "Antigravity",
    "version": (1, 0),
    "blender": (3, 0, 0),
    "location": "View3D > Sidebar > CG2 Tools",
    "description": "Export selected curves to JSON for CG2 Engine",
    "category": "Export",
}

class CG2_PT_SplineExporterPanel(bpy.types.Panel):
    bl_label = "CG2 Spline Export"
    bl_idname = "CG2_PT_SplineExporterPanel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "CG2 Tools"

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        layout.prop(scene, "cg2_export_path")
        layout.operator("export.cg2_spline")


class CG2_OT_ExportSpline(bpy.types.Operator):
    bl_idname = "export.cg2_spline"
    bl_label = "Export to JSON"
    bl_description = "Export selected curves as waypoints to JSON"

    def execute(self, context):
        export_path = context.scene.cg2_export_path
        
        # Ensure directory exists
        os.makedirs(os.path.dirname(export_path), exist_ok=True)
        
        output_data = {"rails": []}
        
        selected_curves = [obj for obj in context.selected_objects if obj.type == 'CURVE']
        
        if not selected_curves:
            self.report({'WARNING'}, "No Curve objects selected!")
            return {'CANCELLED'}
        
        for obj in selected_curves:
            # Apply object transform to get world coordinates
            world_matrix = obj.matrix_world
            
            rail_data = {
                "name": obj.name,
                "points": []
            }
            
            # Extract points from the first spline in the curve
            if len(obj.data.splines) > 0:
                spline = obj.data.splines[0]
                
                points = []
                if spline.type == 'BEZIER':
                    points = spline.bezier_points
                else: # POLY or NURBS
                    points = spline.points
                
                for p in points:
                    # Get world coordinate
                    if spline.type == 'BEZIER':
                        local_co = p.co
                    else:
                        local_co = p.co.xyz # NURBS points have 4D coords (x,y,z,w)
                    
                    world_co = world_matrix @ local_co
                    
                    # Coordinate Conversion: Blender (X, Y, Z) -> Engine (X, Z, Y)
                    # Assuming Blender +X = Right, +Y = Forward, +Z = Up
                    # Engine: +X = Right, +Y = Up, +Z = Forward
                    engine_x = round(world_co.x, 3)
                    engine_y = round(world_co.z, 3)
                    engine_z = round(world_co.y, 3)
                    
                    rail_data["points"].append([engine_x, engine_y, engine_z])
                    
            output_data["rails"].append(rail_data)
            
        try:
            with open(export_path, 'w', encoding='utf-8') as f:
                json.dump(output_data, f, indent=4)
            self.report({'INFO'}, f"Exported {len(selected_curves)} rails to {export_path}")
        except Exception as e:
            self.report({'ERROR'}, f"Failed to export: {str(e)}")
            return {'CANCELLED'}
            
        return {'FINISHED'}


def register():
    bpy.utils.register_class(CG2_PT_SplineExporterPanel)
    bpy.utils.register_class(CG2_OT_ExportSpline)
    
    # Default export path
    default_path = r"C:\Users\K024G\source\repos\CG2\project\resources\levels\splines.json"
    bpy.types.Scene.cg2_export_path = bpy.props.StringProperty(
        name="Export Path",
        description="Path to save the JSON file",
        default=default_path,
        subtype='FILE_PATH'
    )


def unregister():
    bpy.utils.unregister_class(CG2_PT_SplineExporterPanel)
    bpy.utils.unregister_class(CG2_OT_ExportSpline)
    del bpy.types.Scene.cg2_export_path


if __name__ == "__main__":
    register()
