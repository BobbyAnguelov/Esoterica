import bpy, math
import os

from . import ee_runtime, pcg_utils, map_utils

bl_info = {
    'name':         'Esoterica Blender Tools',
    'blender':      (4, 0, 0),
    'category':     'Object',
    'version':       (1, 0, 0),
    'author':       'Esoterica Team',
    'description':  'Tools for working with Esoterica Engine',
}

def select_object(blender_object):
    bpy.ops.object.select_all(action='DESELECT')
    blender_object.select_set(True)
    bpy.context.view_layer.objects.active = blender_object


def is_file_in_folder(file_path, folder_path):
    if file_path.startswith('//'):
        return False
    abs_file_path = os.path.abspath(file_path)
    abs_folder_path = os.path.abspath(folder_path)
    return os.path.commonpath([abs_file_path, abs_folder_path]) == abs_folder_path


def validate_operator_properties(operator, properties):
    """Drop any keyword the installed Blender does not accept for this operator.

    Blender renames and removes operator properties between releases, and passing an unknown keyword
    raises TypeError rather than being ignored. Filtering against the operator's own RNA properties lets
    the export run on a range of versions instead of breaking on one renamed argument.

    Returns the accepted subset. Call this once and reuse the result - the lookup is not cheap.
    """
    supported = set(operator.get_rna_type().properties.keys())
    accepted = { name: value for name, value in properties.items() if name in supported }
    ignored = sorted(set(properties.keys()) - supported)

    if ignored:
        print(f'Esoterica: {operator.idname()} does not accept {", ".join(ignored)} - ignoring')

    return accepted

class ET_EsotericaToolsPreferences(bpy.types.AddonPreferences):
    bl_idname = __name__

    esoterica_path: bpy.props.StringProperty(name='Esoterica path', subtype='DIR_PATH')  # type: ignore

    def draw(self, context):
        self.layout.label(text='EE Tools Settings')
        self.layout.prop(self, 'esoterica_path')


class ET_OT_ExportMeshes(bpy.types.Operator):
    bl_idname = 'eetools.export_meshes'
    bl_label = 'Export meshes'

    def execute(self, context):
        if not bpy.data.filepath:
            self.report({'ERROR'}, "Current blend file is not saved.")
            return {'CANCELLED'}
        
        addon_prefs = context.preferences.addons[__name__].preferences
        esoterica_path = os.path.abspath(addon_prefs.esoterica_path)

        ee_data_path = os.path.join(esoterica_path, 'Data')
        exported_data_path = os.path.join(bpy.path.abspath('//'), 'ExportedData')
        descriptors_path = os.path.join(bpy.path.abspath('//'), 'ResourceDescriptors')

        os.makedirs(exported_data_path, exist_ok=True)

        mesh_descriptors_path = os.path.join(descriptors_path, 'Meshes')
        
        os.makedirs(mesh_descriptors_path, exist_ok=True)

        os.makedirs(os.path.join(exported_data_path, 'textures'), exist_ok=True)

        # Validate fbx export properties upfront
        fbxProperties = validate_operator_properties(bpy.ops.export_scene.fbx, {
            'check_existing': False,
            'use_selection': True,
            'object_types': {'MESH'},
            'use_mesh_modifiers': True,
            'use_triangles': True,
            'apply_scale_options': 'FBX_SCALE_ALL',
            'bake_space_transform': True,
            'mesh_smooth_type': 'FACE',
            'path_mode': 'RELATIVE',
            'embed_textures': False,
            'export_image_format': 'AUTO',
            'export_materials': True,
            'export_texcoords': True,
            'export_normals': True,
            'export_tangents': False,
            'export_animations': False,
        })

        numExported = 0

        for mesh in bpy.data.meshes:
            # Skip meshes without materials
            if len(mesh.materials.items()) == 0:
                continue

            temp_object = bpy.data.objects.new(f'{map_utils.descriptor_path(mesh.name, '')}_Mesh', mesh)
            context.collection.objects.link(temp_object)

            target_path = os.path.join(exported_data_path, f'{mesh.name}.fbx')

            select_object(temp_object)

            bpy.ops.export_scene.fbx(filepath=target_path, **fbxProperties)

            # Clean up
            bpy.data.objects.remove(temp_object, do_unlink=True)

            # ------------------------------------------------------------------------------------------------
            # Write descriptor

            ee_mesh_path = target_path.replace(ee_data_path, 'data:/').replace('\\', '/')

            mesh_descriptor_path = os.path.join(mesh_descriptors_path, map_utils.descriptor_path(mesh.name, '.mesh'))

            mesh_descriptor = None
            if os.path.exists(mesh_descriptor_path):
                mesh_descriptor = ee_runtime.load_object(mesh_descriptor_path)
            else:
                mesh_descriptor = ee_runtime.create_object('EE::Render::StaticMeshResourceDescriptor')

            mesh_descriptor.set_property('m_meshPath', ee_mesh_path)
            mesh_descriptor.set_property('m_meshGroup', 'data://render/meshgroups/highpoly.meshgrp')

            ee_runtime.save_object(mesh_descriptor, mesh_descriptor_path, '1')

            numExported += 1

        self.report({'INFO'}, f'Exported {numExported} mesh(es) as FBX with materials and textures')

        return {'FINISHED'}

class ET_OT_ExportMap(bpy.types.Operator):
    bl_idname = 'eetools.export_map'
    bl_label = 'Export scene as map'

    def execute(self, context):
        if not bpy.data.filepath:
            self.report({'ERROR'}, "Current blend file is not saved.")
            return {'CANCELLED'}
        
        addon_prefs = context.preferences.addons[__name__].preferences
        esoterica_path = os.path.abspath(addon_prefs.esoterica_path)

        ee_data_path = os.path.join(esoterica_path, 'Data')

        exported_data_path = os.path.join(bpy.path.abspath('//'), 'ExportedData')
        descriptors_path = os.path.join(bpy.path.abspath('//'), 'ResourceDescriptors')        
        
        mesh_descriptors_path = os.path.join(descriptors_path, 'Meshes')
        
        os.makedirs(exported_data_path, exist_ok=True)
        os.makedirs(mesh_descriptors_path, exist_ok=True)

        depsgraph = context.evaluated_depsgraph_get()

        # Build entity data
        pretty_xml = map_utils.build_entity_data(self.report, context, depsgraph, ee_data_path, exported_data_path, mesh_descriptors_path)
        if pretty_xml is None:
            return {'CANCELLED'}
        
        with open(bpy.data.filepath.replace('.blend', '.map'), "wb") as map_file:
            map_file.write('<Type TypeID="EE::EntityModel::EntityMapResourceDescriptor" Version="0" />'.encode())
            map_file.write(pretty_xml)

        return {'FINISHED'}

class ET_OT_ExportEntityCollection(bpy.types.Operator):
    bl_idname = 'eetools.export_entity_collection'
    bl_label = 'Export objects as entity collection'

    def execute(self, context):
        if not bpy.data.filepath:
            self.report({'ERROR'}, "Current blend file is not saved.")
            return {'CANCELLED'}

        addon_prefs = context.preferences.addons[__name__].preferences
        esoterica_path = os.path.abspath(addon_prefs.esoterica_path)

        ee_data_path = os.path.join(esoterica_path, 'Data')

        exported_data_path = os.path.join(bpy.path.abspath('//'), 'ExportedData')
        descriptors_path = os.path.join(bpy.path.abspath('//'), 'ResourceDescriptors')
        
        mesh_descriptors_path = os.path.join(descriptors_path, 'Meshes')

        os.makedirs(exported_data_path, exist_ok=True)
        os.makedirs(mesh_descriptors_path, exist_ok=True)

        depsgraph = context.evaluated_depsgraph_get()

        # Build entity data
        map_data = map_utils.build_entity_data(context, depsgraph, ee_data_path, exported_data_path, mesh_descriptors_path)
        if map_data is None:
            return {'CANCELLED'}

        # Use first active object as ECC file name
        ecc_name = 'EntityCollection'
        for scene_object in context.scene.objects:
            if not scene_object.visible_get():
                continue

            ecc_name = scene_object.name
            break
        
        with open(bpy.path.abspath(f'//{ecc_name}.ecc'), "wb") as map_file:
            map_file.write(map_data)

        return {'FINISHED'}

class ET_OT_ExportPCG(bpy.types.Operator):
    bl_idname = 'eetools.export_pcg'
    bl_label = 'Export scene as PCG'

    def execute(self, context):
        self.report({'ERROR'}, "This is not supported and work in progress, don't use it yet.")
        return {'CANCELLED'}
        
        if not bpy.data.filepath:
            self.report({'ERROR'}, "Current blend file is not saved.")
            return {'CANCELLED'}
        
        addon_prefs = context.preferences.addons[__name__].preferences
        esoterica_path = os.path.abspath(addon_prefs.esoterica_path)

        ee_data_path = os.path.join(esoterica_path, 'Data')
        exported_data_path = os.path.join(bpy.path.abspath('//'), 'ExportedData')
        descriptors_path = os.path.join(bpy.path.abspath('//'), 'ResourceDescriptors')

        os.makedirs(exported_data_path, exist_ok=True)

        mesh_descriptors_path = os.path.join(descriptors_path, 'Meshes')

        os.makedirs(mesh_descriptors_path, exist_ok=True)

        depsgraph = context.evaluated_depsgraph_get()
        pcgi_file_path = bpy.data.filepath.replace('.blend', '.pcgi')
        pcgr_file_path = bpy.data.filepath.replace('.blend', '.pcgr')

        pcg_utils.build_pcg_data(depsgraph.object_instances, None, pcgi_file_path, pcgr_file_path, ee_data_path, mesh_descriptors_path)
        
        return {'FINISHED'}


class ET_OT_LoadReflectionData(bpy.types.Operator):
    bl_idname = "eetools.load_reflection_data"
    bl_label = "Load Reflection Data"

    def execute(self, context):
        addon_prefs = context.preferences.addons[__name__].preferences
        esoterica_path = os.path.abspath(addon_prefs.esoterica_path)

        ee_runtime.load_reflection_data(os.path.join(esoterica_path, 'Build/ReflectionData.xml'))

        return {'FINISHED'}


class ET_OT_InstallDependencies(bpy.types.Operator):
    bl_idname = 'eetools.install_dependencies'
    bl_label = 'Install dependencies'

    def execute(self, context):
        # import pip
        # pip.main(['install', 'numpy'])

        return {'FINISHED'}


class ET_PT_EsotericaToolsPanel(bpy.types.Panel):
    bl_label = 'Esoterica Tools'
    bl_category = 'Esoterica Tools'
    bl_idname = 'ET_PT_EsotericaToolsPanel'
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'

    def draw(self, context):
        self.layout.label(text='Tools version: ' + str(bl_info['version']), icon='QUESTION')

        addon_prefs = context.preferences.addons[__name__].preferences
        esoterica_path = os.path.abspath(addon_prefs.esoterica_path)

        box = self.layout.box()

        if esoterica_path is not None:
            box.operator(ET_OT_InstallDependencies.bl_idname)

            box.label(text='Export', icon='PLUGIN')
            box.prop(addon_prefs, 'esoterica_path')

            box.operator(ET_OT_LoadReflectionData.bl_idname)

            box.separator()

            box.operator(ET_OT_ExportMeshes.bl_idname)
            box.operator(ET_OT_ExportMap.bl_idname)

            box.separator()

            box.operator(ET_OT_ExportEntityCollection.bl_idname)
            box.operator(ET_OT_ExportPCG.bl_idname)
        else:
            box.label(text='Addon is not configured!', icon='ERROR')


addon_classes = [
    ET_EsotericaToolsPreferences,
    ET_PT_EsotericaToolsPanel,
    ET_OT_ExportPCG,
    ET_OT_ExportMap,
    ET_OT_ExportEntityCollection,
    ET_OT_LoadReflectionData,
    ET_OT_InstallDependencies,
    ET_OT_ExportMeshes
]


def register():
    for addon_class in addon_classes:
        bpy.utils.register_class(addon_class)


def unregister():

    for addon_class in reversed(addon_classes):
        bpy.utils.unregister_class(addon_class)


if __name__ == '__main__':
    register()
