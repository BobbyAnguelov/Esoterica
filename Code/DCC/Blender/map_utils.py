
import bpy, math
import os

from . import pcg_utils, ee_runtime

import xml.etree.ElementTree as ET

def descriptor_path(path, extension):
    return path.replace('.', '_') + extension

def build_entity_data(report, context, depsgraph, ee_data_path, exported_data_path, mesh_descriptors_path):
    # map_root = ET.Element('Type')
    # map_root.attrib['TypeID'] = 'EE::EntityModel::EntityMapResourceDescriptor'
    # map_root.attrib['Version'] = '0'
    
    # custom_data = ET.Element('CustomData')
    # map_root.append(custom_data)
    
    map_root = ET.Element('CustomData')
    
    entities = ET.Element('Entities')
    map_root.append(entities)
    
    name_ids = {}
    
    for instance in depsgraph.object_instances:
        instance_object = None
        instance_parent = None
        instance_mesh = None
    
        if instance.is_instance:
            instance_parent = instance.parent
            instance_object = instance.instance_object
        else:
            instance_object = instance.object

        if instance_object.type != 'MESH':
            print(f'SKIP, not a mesh: {instance_object.name}')
            continue
        
        instance_mesh = instance_object.original.data if instance_object.original else instance_object.data
        
        if len(instance_mesh.materials.items()) == 0:
            print(f'SKIP, no materials: {instance_object.name}')
            continue
        
        print(f'{instance_object.name} -> f{instance_parent} / {instance_mesh.name}')
        
        entity_element = None
        if instance_parent:
            entity_element = entities.find(f'.//Entity[@Name="{instance_parent.name}"]');
            if entity_element is None:
                entity_element = ET.Element('Entity', Name=instance_parent.name)
                entities.append(entity_element)
        else:
            entity_element = ET.Element('Entity', Name=instance_object.name)
            entities.append(entity_element)

        entity_components = entity_element.find(f'Components')
        if entity_components is None:
            entity_components = ET.Element('Components')
            entity_element.append(entity_components)

        entity_resources = entity_element.find('ReferencedResources')
        if entity_resources is None:
            entity_resources = ET.Element('ReferencedResources')
            entity_element.append(entity_resources)

        # Create spatial root component
        matrix_world = instance.matrix_world        
        (position, rotation, scale) = matrix_world.decompose()
        rotation = rotation.to_euler()
        rotation[0] = math.degrees(rotation[0])
        rotation[1] = math.degrees(rotation[1])
        rotation[2] = math.degrees(rotation[2])

        component_transform = f'{rotation[0]},{rotation[1]},{rotation[2]},{position[0]},{position[1]},{position[2]},1.0'
        component_local_scale = f'{scale[0]},{scale[1]},{scale[2]}'

        spatial_root = entity_components.find('.//Type[@TypeID="EE::SpatialEntityComponent"]')
        if spatial_root is None:
            spatial_root = ee_runtime.create_object('EE::SpatialEntityComponent')
            spatial_root.set_property('m_name', 'SpatialRoot')
            # TODO: Figure this out
            # spatial_root.set_property('m_transform', entity_transform)
            entity_components.append(spatial_root.to_xml_element(use_map_format=True))

        # Create referenced resources
        mesh_descriptor_path = os.path.join(mesh_descriptors_path, descriptor_path(instance_mesh.name, '.mesh'))
        mesh_descriptor_path = mesh_descriptor_path.replace(ee_data_path, 'data:/').replace('\\', '/')

        resource_element = entity_resources.find(f'.//Resource[@ID="{mesh_descriptor_path}"]')
        if resource_element is None:
            entity_resources.append(ET.Element('Resource', ID=mesh_descriptor_path))

        id_counter = name_ids.get(instance_object.name)
        if id_counter is None:
            id_counter = 0
        else:
            id_counter = id_counter + 1
        name_ids[instance_object.name] = id_counter

        static_mesh_component = ee_runtime.create_object('EE::Render::StaticMeshComponent')
        static_mesh_component.set_property('m_name', f'{instance_object.name}_{id_counter}')
        static_mesh_component.set_property('m_mesh', mesh_descriptor_path)
        static_mesh_component.set_property('m_transform', component_transform)
        static_mesh_component.set_property('m_nonUniformScale', component_local_scale)

        static_mesh_element = static_mesh_component.to_xml_element(use_map_format=True)
        static_mesh_element.set('SpatialParent', 'SpatialRoot')

        entity_components.append(static_mesh_element)
                        
    # Save map
    ET.indent(map_root)
    pretty_xml = ET.tostring(map_root, encoding="utf-8", xml_declaration=False)

    return pretty_xml
