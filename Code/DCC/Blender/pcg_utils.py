import bpy, math, mathutils
import os, struct

from . import ee_runtime

def build_pcg_data(blender_instances, blender_parent, pcgi_file_path, pcgr_file_path, ee_data_path, mesh_descriptors_path):
        exported_instances = []
        instance_name_mapping = {}

        for instance in blender_instances:
            instance_object = None
            if instance.is_instance:
                if blender_parent is not None:
                    if instance.parent != blender_parent:
                        continue

                instance_object = instance.instance_object
            else:
                continue

            if instance_object.type != 'MESH':
                continue

            matrix_world = instance.matrix_world
            if blender_parent is not None:
                matrix_world = blender_parent.matrix_world.inverted() @ matrix_world

            (position, rotation, scale) = matrix_world.decompose()
            rotation = rotation.to_euler()
            rotation[0] = math.degrees(rotation[0])
            rotation[1] = math.degrees(rotation[1])
            rotation[2] = math.degrees(rotation[2])

            if not instance.instance_object.name in instance_name_mapping:
                mesh_descriptor_path = os.path.join(mesh_descriptors_path, f'{instance.instance_object.data.name}.mesh')
                if not os.path.exists(mesh_descriptor_path):
                    continue

                ee_name_mapping = ee_runtime.create_object('EE::Render::PCGInstanceNameMapping')
                ee_name_mapping.set_property('m_instanceName', instance.instance_object.name)
                ee_name_mapping.set_property('m_instanceMesh', mesh_descriptor_path.replace(ee_data_path, 'data:/').replace('\\', '/'))

                instance_name_mapping[instance.instance_object.name] = ee_name_mapping

            exported_instances.append((str(instance.instance_object.name), position[0], position[1], position[2], rotation[0], rotation[1], rotation[2], scale[0], scale[1], scale[2]))

        if not exported_instances:
            return False

        with open(pcgi_file_path, 'wb') as output_file:
            output_file.write(struct.pack('cccc', b'P', b'C', b'G', b'I'))
            output_file.write(struct.pack('I', 1))
            output_file.write(struct.pack('I', len(exported_instances)))
            for (name, position_x, position_y, position_z, rotation_x, rotation_y, rotation_z, scale_x, scale_y, scale_z) in exported_instances:
                name_buffer = name.encode('utf-8')
                output_file.write(struct.pack('I', len(name_buffer)))
                output_file.write(name_buffer)
                output_file.write(struct.pack('fffffffff', position_x, position_y, position_z,
                                  rotation_x, rotation_y, rotation_z, scale_x, scale_y, scale_z))
        
        pcg_descriptor = None
        if os.path.exists(pcgr_file_path):
            pcg_descriptor = ee_runtime.load_object(pcgr_file_path)
        else:
            pcg_descriptor = ee_runtime.create_object('EE::Render::PCGResourceDescriptor')

        pcg_descriptor.set_property('m_pcgFilePath', pcgi_file_path.replace(ee_data_path, 'data:/').replace('\\', '/'))
        pcg_descriptor.set_property('m_instanceNameMapping', list(instance_name_mapping.values()))

        ee_runtime.save_object(pcg_descriptor, pcgr_file_path, '0')

        return True
