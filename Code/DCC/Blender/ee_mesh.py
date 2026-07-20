
import bpy

class ET_EsotericaMeshPropertyGroup(bpy.types.PropertyGroup):
    mesh_path:  bpy.props.StringProperty( name = 'Path', subtype = 'FILE_PATH' )
    mesh_group: bpy.props.StringProperty( name = 'Group', default = 'data://render/meshgroups/default.meshgrp' )

class ET_PT_EsotericaMeshPropertyPanel(bpy.types.Panel):
    bl_label        = "Esoterica Mesh"
    bl_idname       = "ET_PT_EsotericaMeshPropertyPanel"
    bl_space_type   = 'PROPERTIES'
    bl_region_type  = 'WINDOW'
    bl_context      = "collection"

    def draw(self, context):
        self.layout.prop(context.collection.ee_mesh_properties, 'mesh_path')
        self.layout.prop(context.collection.ee_mesh_properties, 'mesh_group')
