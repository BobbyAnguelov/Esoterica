import xml.etree.ElementTree as ET


class ReflectionError(Exception):
    pass


class ReflectedObject:
    def __init__(self, type_name, prop_defs):
        self.type_name = type_name
        self.prop_defs = prop_defs
        self.props = {}

    def set_property(self, name, value):
        if name not in self.prop_defs:
            raise ReflectionError(f'Property "{name}" is not defined for type "{self.type_name}"')

        if self.prop_defs[name].get('is_dynamic_array', False):
            if not isinstance(value, list):
                raise ReflectionError(f'Dynamic array property "{name}" must be a list.')

        self.props[name] = value

    def get_property(self, name):
        if name not in self.prop_defs:
            raise ReflectionError(f'Property "{name}" is not defined for type "{self.type_name}"')
        return self.props.get(name, None)
    
    def to_xml_element(self, use_map_format=False):
        type_element = ET.Element('Type', TypeID=self.type_name)
        for property_name, property_value in self.props.items():
            if isinstance(property_value, list):
                array_element = ET.Element('Property', Path=property_name) if use_map_format else ET.Element('Property', ID=property_name)
                for index, element_value in enumerate(property_value):
                    if isinstance(element_value, ReflectedObject):
                        object_element = element_value.to_xml_element()
                        object_element.set('Index', str(index))
                        array_element.append(object_element)
                    else:
                        ET.SubElement(array_element, 'Property', Index=str(index), Value=str(element_value))
                type_element.append(array_element)
            else:
                if isinstance(property_value, ReflectedObject):
                    object_element = property_value.to_xml_element()
                    object_element.set('ID', property_name)
                    type_element.append(object_element)
                else:
                    ET.SubElement(type_element, 'Property', Path=property_name, Value=str(property_value)) if use_map_format else ET.SubElement(type_element, 'Property', ID=property_name, Value=str(property_value))
        return type_element

    @classmethod
    def from_xml_element(cls, elem):
        type_name = elem.get('TypeID')
        if not type_name:
            raise ReflectionError('XML element missing "TypeID" attribute.')
        obj = create_object(type_name)

        for prop_elem in elem.findall('Property'):
            property_name = prop_elem.get('ID')
            if not property_name:
                continue
            
            prop_def = obj.prop_defs.get(property_name, {})
            child_value_props = prop_elem.findall('Property')
            child_type_props = prop_elem.findall('Type')

            if child_value_props:
                prop_def['is_dynamic_array'] = True
                array_values = []
                child_value_props = prop_elem.findall('Property')

                sorted_children = sorted(child_value_props, key=lambda x: int(x.get('Index', '0')))
                for child in sorted_children:
                    child_value = child.get('Value')
                    array_values.append(child_value)

                obj.set_property(property_name, array_values)
            elif child_type_props:
                prop_def['is_dynamic_array'] = True
                array_values = []
                child_type_props = prop_elem.findall('Type')

                sorted_children = sorted(child_type_props, key=lambda x: int(x.get('Index', '0')))
                for child in sorted_children:
                    array_values.append(ReflectedObject.from_xml_element(child))

                obj.set_property(property_name, array_values)
            else:
                if prop_def.get('is_dynamic_array', False):
                    raise ReflectionError(f'Property {property_name} is an array but XML data is not an array')

                property_value = prop_elem.get('Value')
                obj.set_property(property_name, property_value)

        for type_element in elem.findall('Type'):
            obj.set_property(type_element.get('ID'), ReflectedObject.from_xml_element(type_element))

        return obj


reflection_types = {}


def load_reflection_data(xml_file_path):
    reflection_types.clear()

    tree = ET.parse(xml_file_path)
    root = tree.getroot()
    for type_element in root.findall('Type'):
        type_name = type_element.get('ID')
        if not type_name:
            continue
        prop_defs = {}
        for prop_elem in type_element.findall('Property'):
            prop_id = prop_elem.get('ID')
            prop_type = prop_elem.get('Type')
            is_dynamic_array = prop_elem.get('IsDynamicArray', 'false').lower() == 'true'
            if prop_id:
                prop_defs[prop_id] = {
                    'type': prop_type,
                    'is_dynamic_array': is_dynamic_array
                }
        reflection_types[type_name] = prop_defs


def create_object(type_name):
    if type_name not in reflection_types:
        raise ReflectionError(
            f'Type "{type_name}" not defined in reflection data')
    return ReflectedObject(type_name, reflection_types[type_name])


def load_object(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()
    return ReflectedObject.from_xml_element(root)

def save_object(object, file_path, version):
    root = object.to_xml_element()
    root.set('Version', version)
    
    ET.indent(root)
    pretty_xml = ET.tostring(root, encoding="utf-8", xml_declaration=False)
    
    with open(file_path, "wb") as f:
        f.write(pretty_xml)
