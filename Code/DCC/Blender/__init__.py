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

def find_linked_node(input, node_type):
    node_stack = []

    if input.is_linked:
        node_stack.append(input.links[0].from_node)

    while len(node_stack) > 0:
        node = node_stack.pop()
        if node.type == node_type:
            return node
        
        if node.type == 'GROUP':
            for group_output in node.outputs:
                if group_output.type == 'SHADER':
                    # HACK: Just assume that this is our BRDF node...
                    return node

            # for group_node in node.node_tree.nodes:
            #     if group_node.type == 'GROUP_OUTPUT':
            #         for group_input in group_node.inputs:
            #             if group_input.is_linked:
            #                 node_stack.append(group_input.links[0].from_node)
                            
        for node_input in node.inputs:
            if node_input.is_linked:
                node_stack.append(node_input.links[0].from_node)

    return None

def find_linked_texture(input, ee_data_path, texture_descriptors_path):
    if input is None:
        return (None, None, None)
    
    # Allow EXR for emission outputs
    allow_exr = input.name == 'Emission'

    # Try to figure out which textures material is using.
    # For each relevant property we walk back the nodes and choose the first texture node that we found.
    node_stack = []
    if input.is_linked:
        node_stack.append(input.links[0].from_node)

    while len(node_stack) > 0:
        node = node_stack.pop()

        if node.type == 'TEX_IMAGE':
            # Some people use .EXR files for normal maps and roughness maps.
            # I don't know why, but we need to maintain pipeline sanity.
            filepath = node.image.filepath
            old_filepath = node.image.filepath_raw
            old_fileformat = node.image.file_format

            if filepath:
                # Relative paths outside of the current folder needs to be fixed?
                # Copy everything to ExportedData to be self-contained.
                if filepath.startswith('//..'):
                    filepath = filepath.replace('//', '//ExportedData/').replace('..\\', '')

                if not allow_exr and node.image.file_format == 'OPEN_EXR':
                    filepath = filepath.replace('.exr', '.png')

                abs_path = bpy.path.abspath(filepath)
                if not os.path.exists(abs_path):
                    print(f'Resaving image {node.image.filepath} -> {abs_path}')

                    if not allow_exr and node.image.file_format == 'OPEN_EXR':
                        node.image.file_format = 'PNG'

                    node.image.filepath_raw = abs_path
                    node.image.save()

                    node.image.filepath_raw = old_filepath
                    node.image.file_format = old_fileformat

                descriptor_path = os.path.join(texture_descriptors_path, map_utils.descriptor_path(node.image.name, '.texture'))
                ee_path = descriptor_path.replace(ee_data_path, 'data:/').replace( '\\', '/' )
                return (ee_path, descriptor_path, abs_path)

        for node_input in node.inputs:
            if node_input.is_linked:
                node_stack.append(node_input.links[0].from_node)

    return (None, None, None)

class ET_EsotericaToolsPreferences(bpy.types.AddonPreferences):
    bl_idname = __name__

    esoterica_path: bpy.props.StringProperty(name='Esoterica path', subtype='DIR_PATH')  # type: ignore
    allow_blend_file_edits: bpy.props.BoolProperty(name='Allow editing this .blend file') # type: ignore

    def draw(self, context):
        self.layout.label(text='EE Tools Settings')
        self.layout.prop(self, 'esoterica_path')
        self.layout.prop(self, 'allow_blend_file_edits')


class ET_OT_ExportMaterials(bpy.types.Operator):
    bl_idname = 'eetools.export_materials'
    bl_label = 'Export materials'

    def execute(self, context):
        if not bpy.data.filepath:
            self.report({'ERROR'}, "Current blend file is not saved.")
            return {'CANCELLED'}
        
        addon_prefs = context.preferences.addons[__name__].preferences
        esoterica_path = os.path.abspath(addon_prefs.esoterica_path)
        allow_blend_file_edits = addon_prefs.allow_blend_file_edits
        
        if not allow_blend_file_edits:
            self.report({'ERROR'}, f'Material export MAY need to modify the current blend file.\nFor example, if you use .exr roughness textures the addon will convert that to .png and update the materials.\nEnable "{ET_EsotericaToolsPreferences.bl_rna.properties['allow_blend_file_edits'].name}" checkbox explicitly to allow this.')
            return {'CANCELLED'}

        ee_data_path = os.path.join(esoterica_path, 'Data')
        exported_data_path = os.path.join(bpy.path.abspath('//'), 'ExportedData')
        descriptors_path = os.path.join(bpy.path.abspath('//'), 'ResourceDescriptors')

        os.makedirs(exported_data_path, exist_ok=True)

        material_descriptors_path = os.path.join(descriptors_path, 'Materials')
        texture_descriptors_path = os.path.join(descriptors_path, 'Textures')

        os.makedirs(material_descriptors_path, exist_ok=True)
        os.makedirs(texture_descriptors_path, exist_ok=True)

        for material in bpy.data.materials:
            if material.users == 0:
                continue

            # Don't care about grease pencil materials
            if material.is_grease_pencil:
                continue

            # Don't care about OSL and other material types
            if not material.use_nodes:
                continue

            output_node = next((node for node in material.node_tree.nodes if node.type == 'OUTPUT_MATERIAL'), None)
            if output_node is None:
                continue

            surface_input = output_node.inputs.get('Surface')
            if surface_input is None:
                continue

            principled_bsdf = find_linked_node(surface_input, 'BSDF_PRINCIPLED')
            if principled_bsdf is None:
                continue

            material_descriptor_path = os.path.join(material_descriptors_path, map_utils.descriptor_path(material.name, '.material'))

            material_descriptor = None
            if os.path.exists(material_descriptor_path):
                material_descriptor = ee_runtime.load_object(material_descriptor_path)
            else:
                material_descriptor = ee_runtime.create_object('EE::Render::MaterialResourceDescriptor')

            material_descriptor.set_property('m_shader', 'ComplexSurfacePBR')
            
            shader_parameters = material_descriptor.get_property('m_shaderParameters')
            if shader_parameters is None:
                shader_parameters = ee_runtime.create_object('EE::Render::Shaders::ComplexSurfacePBRParameters')
            
            material_has_alpha = False
            shader_flags = ''
            if material.blend_method == 'OPAQUE':
                pass                
            elif material.blend_method == 'CLIP' or material.blend_method == 'HASHED':
                material_has_alpha = True
                shader_flags += '|AlphaTest'
            elif material.blend_method == 'BLEND':
                material_has_alpha = True
                shader_flags += '|AlphaBlend'
            
            if not material.use_backface_culling:
                shader_flags += '|TwoSided'
                
            # TODO: Handle alpha blended materials
            shader_parameters.set_property('m_opacity', '1.0')
            shader_parameters.set_property('m_shaderFlags', shader_flags.lstrip('|'))
            
            # HACK: Extremely cursed way of guessing node inputs, since we don't know what node type this is
            base_color_input = principled_bsdf.inputs.get('Base Color', None)
            if base_color_input is None:
                base_color_input = principled_bsdf.inputs.get('Diffuse', None)

            alpha_input = principled_bsdf.inputs.get('Alpha', None)

            normal_input = principled_bsdf.inputs.get('Normal', None)

            roughness_input = principled_bsdf.inputs.get('Roughness', None)
            if roughness_input is None:
                roughness_input = principled_bsdf.inputs.get('Rough', None)

            metallic_input = principled_bsdf.inputs.get('Metallic', None)

            # Export albedo (with optional mask if it's separate)
            (ee_mask_path, mask_path, mask_file) = find_linked_texture(alpha_input, ee_data_path, texture_descriptors_path)
            (ee_albedo_path, albedo_path, albedo_file) = find_linked_texture(base_color_input, ee_data_path, texture_descriptors_path)
            if ee_albedo_path is not None:
                shader_parameters.set_property('m_albedoTexture', ee_albedo_path)

                albedo_texture = None
                if os.path.exists(albedo_path):
                    albedo_texture = ee_runtime.load_object(albedo_path)
                else:
                    albedo_texture = ee_runtime.create_object('EE::Render::TextureResourceDescriptor')
                
                albedo_texture.set_property('m_name', f'{map_utils.descriptor_path(material.name, '')}_Albedo')
                
                source_paths = None
                if material_has_alpha and ee_mask_path is not None and ee_mask_path != ee_albedo_path:
                    albedo_texture.set_property('m_textureGroup', 'data://render/texturegroups/albedotexturewithmask.txtg')
                    source_paths = [
                        albedo_file.replace(ee_data_path, 'data:/').replace('\\', '/'),
                        mask_file.replace(ee_data_path, 'data:/').replace('\\', '/')
                    ]
                else:
                    albedo_texture.set_property('m_textureGroup', 'data://render/texturegroups/albedotexture.txtg')
                    source_paths = [albedo_file.replace(ee_data_path, 'data:/').replace('\\', '/')]
                
                albedo_texture.set_property('m_sourcePaths', source_paths)
                ee_runtime.save_object(albedo_texture, albedo_path, '0')
            
            (ee_normal_path, normal_path, normal_file) = find_linked_texture(normal_input, ee_data_path, texture_descriptors_path)
            if ee_normal_path is not None:
                shader_parameters.set_property('m_normalTexture', ee_normal_path)

                normal_texture = None
                if os.path.exists(normal_path):
                    normal_texture = ee_runtime.load_object(normal_path)
                else:
                    normal_texture = ee_runtime.create_object('EE::Render::TextureResourceDescriptor')
                    normal_texture.set_property('m_textureGroup', 'data://render/texturegroups/normaltexture.txtg')
                    normal_texture.set_property('m_name', f'{map_utils.descriptor_path(material.name, '')}_Normal')
                
                normal_texture.set_property('m_sourcePaths', [normal_file.replace(ee_data_path, 'data:/').replace('\\', '/')])
                ee_runtime.save_object(normal_texture, normal_path, '0')

            (ee_roughness_path, roughness_path, roughness_file) = find_linked_texture(roughness_input, ee_data_path, texture_descriptors_path)
            (ee_metalness_path, metalness_path, metalness_file) = find_linked_texture(metallic_input, ee_data_path, texture_descriptors_path)

            ee_pbr_path = None
            if ee_roughness_path:
                ee_pbr_path = ee_roughness_path
                pbr_path = roughness_path
            elif ee_metalness_path:
                ee_pbr_path = ee_metalness_path
                pbr_path = metalness_path
            
            if ee_pbr_path:
                shader_parameters.set_property('m_pbrTexture', ee_pbr_path)

                pbr_texture = None
                if os.path.exists(pbr_path):
                    pbr_texture = ee_runtime.load_object(pbr_path)
                else:
                    pbr_texture = ee_runtime.create_object('EE::Render::TextureResourceDescriptor')
                    pbr_texture.set_property('m_textureGroup', 'data://render/texturegroups/pbrtexture.txtg')
                    pbr_texture.set_property('m_name', f'{map_utils.descriptor_path(material.name, '')}_PBR')
                
                pbr_source_paths = [
                    'data://render/textures/white_16x16.png',
                    'data://render/textures/black_16x16.png',
                    'data://render/textures/white_16x16.png',
                    'data://render/textures/white_16x16.png'
                ]

                if roughness_file is not None:
                    pbr_source_paths[0] = roughness_file.replace(ee_data_path, 'data:/').replace('\\', '/')
                if metalness_file is not None:
                    pbr_source_paths[1] = metalness_file.replace(ee_data_path, 'data:/').replace('\\', '/')

                pbr_texture.set_property('m_sourcePaths', pbr_source_paths)
                ee_runtime.save_object(pbr_texture, pbr_path, '0')

            material_descriptor.set_property('m_shaderParameters', shader_parameters)
            ee_runtime.save_object(material_descriptor, material_descriptor_path, '0')

        return {'FINISHED'}

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
        material_descriptors_path = os.path.join(descriptors_path, 'Materials')

        os.makedirs(mesh_descriptors_path, exist_ok=True)
        os.makedirs(material_descriptors_path, exist_ok=True)

        for mesh in bpy.data.meshes:
            # Skip meshes without materials
            if len(mesh.materials.items()) == 0:
                continue

            temp_object = bpy.data.objects.new(f'{map_utils.descriptor_path(mesh.name, '')}_Mesh', mesh)
            context.collection.objects.link(temp_object)

            target_path = os.path.join(exported_data_path, f'{mesh.name}.glb')

            select_object(temp_object)
            bpy.ops.export_scene.gltf(filepath=target_path,
                                      check_existing=False,
                                      use_active_scene=False,
                                      use_active_collection=False,
                                      use_active_collection_with_nested=False,
                                      use_selection=True,
                                      export_animations=False,
                                      export_optimize_animation_size=False,
                                      export_format='GLB',
                                      export_materials='EXPORT',
                                      export_texcoords=True,
                                      export_normals=True,
                                      export_tangents=False,
                                      export_vertex_color='ACTIVE',
                                      export_cameras=False,
                                      export_image_format='NONE')

            # FBX is slower because it exports what we don't need
            # target_path = os.path.join( exported_data_path, f'{mesh.name}.fbx' )
            # bpy.ops.export_scene.fbx( filepath             = target_path,
            #                           check_existing       = False,
            #                           use_selection        = True,
            #                           object_types         = {'MESH'},
            #                           use_triangles        = True,
            #                           embed_textures       = False,
            #                           axis_forward         = 'Y',
            #                           axis_up              = 'Z' )

            # Clean up
            bpy.data.objects.remove(temp_object, do_unlink=True)

            # ------------------------------------------------------------------------------------------------
            # Write descriptors

            ee_mesh_path = target_path.replace(ee_data_path, 'data:/').replace('\\', '/')
            ee_materials_path = material_descriptors_path.replace(ee_data_path, 'data:/').replace('\\', '/')

            mesh_descriptor_path = os.path.join(mesh_descriptors_path, map_utils.descriptor_path(mesh.name, '.mesh'))

            mesh_descriptor = None
            if os.path.exists(mesh_descriptor_path):
                mesh_descriptor = ee_runtime.load_object(mesh_descriptor_path)
            else:
                mesh_descriptor = ee_runtime.create_object('EE::Render::StaticMeshResourceDescriptor')

            mesh_descriptor.set_property('m_meshPath', ee_mesh_path)
            mesh_descriptor.set_property('m_meshGroup', 'data://render/meshgroups/highpoly.meshgrp')

            material_mappings = []
            for material in mesh.materials:
                if material is None:
                    continue
                
                for lod_mask in range(0, 8):
                    material_mapping = ee_runtime.create_object('EE::Render::MeshMaterialMapping')
                    material_mapping.set_property('m_mappingID', f'{map_utils.descriptor_path(mesh.name, '')}_Mesh/{map_utils.descriptor_path(material.name, '')}/{2**lod_mask}')
                    material_mapping.set_property('m_material', f'{ee_materials_path}/{map_utils.descriptor_path(material.name, '.material')}')
                    material_mappings.append(material_mapping)

            mesh_descriptor.set_property('m_materialMappings', material_mappings)

            ee_runtime.save_object(mesh_descriptor, mesh_descriptor_path, '1')

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
            box.prop(addon_prefs, 'allow_blend_file_edits')

            box.operator(ET_OT_LoadReflectionData.bl_idname)

            box.separator()

            box.operator(ET_OT_ExportMaterials.bl_idname)
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
    ET_OT_ExportMeshes,
    ET_OT_ExportMaterials
]


def register():
    for addon_class in addon_classes:
        bpy.utils.register_class(addon_class)


def unregister():

    for addon_class in reversed(addon_classes):
        bpy.utils.unregister_class(addon_class)


if __name__ == '__main__':
    register()
