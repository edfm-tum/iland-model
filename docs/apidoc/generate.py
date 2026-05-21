import os
import re
import sys

# Paths relative to the script location
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SOURCE_API_DIR = os.path.join(SCRIPT_DIR, "../../src/apidoc/iLand")
SOURCE_ABE_DIR = os.path.join(SCRIPT_DIR, "../../src/abe-lib")
SOURCE_ABE_DOC_DIR = os.path.join(SCRIPT_DIR, "../../src/apidoc/ABE")
OUTPUT_DIR = os.path.join(SCRIPT_DIR, "./classes")

# Ensure output directory exists
os.makedirs(OUTPUT_DIR, exist_ok=True)

def replace_crosslinks(text):
    """
    Translates YUI crossLinks to Markdown.
    Example: {{#crossLink "Grid/load:method"}}{{/crossLink}} -> [load()](grid.qmd#load)
    """
    pattern = re.compile(r'\{\{#crossLink\s+"([^"]+)"\}\}(.*?)\{\{/crossLink\}\}', re.DOTALL)
    def repl(m):
        link_target = m.group(1)
        label = m.group(2).strip()
        
        parts = link_target.split('/')
        class_name = parts[0].lower()
        member_name = None
        member_type = None
        
        if len(parts) > 1:
            member_part = parts[1]
            if ':' in member_part:
                member_name, member_type = member_part.split(':')
            else:
                member_name = member_part
                
        # Resolve target URL (assuming classes are in the same folder)
        class_map = {
            "barkbeetle": "../../wiki/barkbeetle-module.qmd",
            "wind": "../../wiki/wind-module.qmd",
            "lib.helper": "helper.qmd",
            "mapgrid": "map.qmd",
            "scheduler": "schedule.qmd",
            "patch": "patches.qmd",
            "scriptgrid": "grid.qmd",
        }
        
        if class_name in class_map:
            url = class_map[class_name]
        else:
            url = f"{class_name}.qmd"
            
        if member_name:
            # Anchor should be lowercased
            url += f"#{member_name.lower()}"
            
        if not label:
            if member_name:
                if member_type == "method" or "method" in link_target:
                    label = f"{member_name}()"
                else:
                    label = member_name
            else:
                label = parts[0]
                
        return f"[{label}]({url})"
    return pattern.sub(repl, text)

def convert_indented_code_blocks(text):
    """
    Finds contiguous lines indented by 4 or more spaces or a tab in descriptions
    and wraps them in standard javascript fenced code blocks.
    """
    lines = text.split('\n')
    new_lines = []
    in_block = False
    block_lines = []
    
    for line in lines:
        stripped = line.strip()
        is_indented = line.startswith('    ') or line.startswith('\t')
        
        if is_indented and stripped:
            if not in_block:
                in_block = True
                block_lines = [line[4:] if line.startswith('    ') else line[1:]]
            else:
                block_lines.append(line[4:] if line.startswith('    ') else line[1:])
        elif not stripped and in_block:
            block_lines.append('')
        else:
            if in_block:
                while block_lines and block_lines[-1] == '':
                    block_lines.pop()
                new_lines.append("```javascript")
                new_lines.extend(block_lines)
                new_lines.append("```")
                in_block = False
                block_lines = []
            new_lines.append(line)
            
    if in_block:
        while block_lines and block_lines[-1] == '':
            block_lines.pop()
        new_lines.append("```javascript")
        new_lines.extend(block_lines)
        new_lines.append("```")
        
    return '\n'.join(new_lines)


def parse_comment_block(block):
    """
    Parses a single comment block and extracts YUI tags.
    """
    # Clean leading * from the block while preserving internal markdown indentation
    lines = []
    for line in block.split('\n'):
        line = line.rstrip()
        match = re.match(r'^(\s*)\*(?: (.*)|(.*))?$', line)
        if match:
            content = match.group(2) if match.group(2) is not None else (match.group(3) or '')
            lines.append(content)
        else:
            if line.strip() == '':
                lines.append('')
            else:
                lines.append(line)
        
    content = "\n".join(lines).strip()
    
    # Extract tags
    tags = {}
    
    # 1. Class tag
    class_match = re.search(r'@class\s+(\w+)', content)
    if class_match:
        tags['class'] = class_match.group(1)
        
    # 2. Method tag
    method_match = re.search(r'@method\s+(\w+)', content)
    if method_match:
        tags['method'] = method_match.group(1)
        
    # 3. Property tag
    prop_match = re.search(r'@property\s+(\w+)', content)
    if prop_match:
        tags['property'] = prop_match.group(1)
        
    # 4. Type tag
    type_match = re.search(r'@type\s+(\w+)', content)
    if type_match:
        tags['type'] = type_match.group(1)
        
    # 5. ReadOnly tag
    if '@readOnly' in content or '@readonly' in content:
        tags['readonly'] = True
        
    # 6. Params tags
    # Matches: @param {type} name description
    param_matches = re.finditer(r'@param\s+\{(\w+)\}\s+(\w+)\s+(.*?)(?=\n@|\Z)', content, re.DOTALL)
    params = []
    for pm in param_matches:
        params.append({
            'type': pm.group(1),
            'name': pm.group(2),
            'desc': pm.group(3).strip()
        })
    if params:
        tags['params'] = params
        
    # 7. Return tag
    # Matches: @return {type} description
    return_match = re.search(r'@return\s+\{(\w+)\}\s+(.*?)(?=\n@|\Z)', content, re.DOTALL)
    if return_match:
        tags['return'] = {
            'type': return_match.group(1),
            'desc': return_match.group(2).strip()
        }
        
    # 8. Example tag
    example_match = re.search(r'@Example\s*(.*?)(?=\n@|\Z)', content, re.DOTALL)
    if not example_match:
        example_match = re.search(r'@example\s*(.*?)(?=\n@|\Z)', content, re.DOTALL)
    if example_match:
        tags['example'] = example_match.group(1).strip()
        
    # Determine description (text before any @ tag)
    desc_lines = []
    for line in lines:
        if line.strip().startswith('@'):
            break
        desc_lines.append(line)
    tags['description'] = "\n".join(desc_lines).strip()
    
    return tags

def extract_comments_from_file(file_path):
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    # Match all /** ... */ comment blocks
    blocks = []
    pattern = re.compile(r'/\*\*(.*?)\*/', re.DOTALL)
    for match in pattern.finditer(content):
        blocks.append(match.group(1))
    return blocks

def generate_docs():
    # Scan both Directories
    source_files = []
    
    if os.path.exists(SOURCE_API_DIR):
        for f in os.listdir(SOURCE_API_DIR):
            if f.endswith('.js'):
                source_files.append(os.path.join(SOURCE_API_DIR, f))
                
    if os.path.exists(SOURCE_ABE_DOC_DIR):
        for f in os.listdir(SOURCE_ABE_DOC_DIR):
            if f.endswith('.js'):
                source_files.append(os.path.join(SOURCE_ABE_DOC_DIR, f))
                
    if os.path.exists(SOURCE_ABE_DIR):
        for f in os.listdir(SOURCE_ABE_DIR):
            if f.endswith('.js'):
                source_files.append(os.path.join(SOURCE_ABE_DIR, f))
        # Support deep directories in ABE
        for root, dirs, files in os.walk(SOURCE_ABE_DIR):
            for f in files:
                if f.endswith('.js'):
                    fp = os.path.join(root, f)
                    if fp not in source_files:
                        source_files.append(fp)
                        
    classes = {}
    
    print(f"Parsing {len(source_files)} Javascript files for YUI documentation...")
    
    for file_path in source_files:
        blocks = extract_comments_from_file(file_path)
        for b in blocks:
            tags = parse_comment_block(b)
            
            # If this is a class header
            if 'class' in tags:
                class_name = tags['class']
                if class_name not in classes:
                    classes[class_name] = {
                        'name': class_name,
                        'description': tags['description'],
                        'properties': [],
                        'methods': []
                    }
                else:
                    # Update description if it was empty
                    if not classes[class_name]['description']:
                        classes[class_name]['description'] = tags['description']
                        
            # If it's a property
            elif 'property' in tags:
                prop_name = tags['property']
                # Try to guess class by looking at file context, or associate with the last seen class
                # For safety, let's store it as a temporary item, and we'll attach it based on context.
                # Actually, YUI docs usually define the class in the same file.
                tags['name'] = prop_name
                # Let's find class name from comments in the same file
                file_blocks = extract_comments_from_file(file_path)
                file_class = None
                for fb in file_blocks:
                    fb_tags = parse_comment_block(fb)
                    if 'class' in fb_tags:
                        file_class = fb_tags['class']
                        break
                
                if file_class:
                    if file_class not in classes:
                        classes[file_class] = {'name': file_class, 'description': '', 'properties': [], 'methods': []}
                    classes[file_class]['properties'].append(tags)
                    
            # If it's a method
            elif 'method' in tags:
                method_name = tags['method']
                tags['name'] = method_name
                
                file_blocks = extract_comments_from_file(file_path)
                file_class = None
                for fb in file_blocks:
                    fb_tags = parse_comment_block(fb)
                    if 'class' in fb_tags:
                        file_class = fb_tags['class']
                        break
                        
                if file_class:
                    if file_class not in classes:
                        classes[file_class] = {'name': file_class, 'description': '', 'properties': [], 'methods': []}
                    classes[file_class]['methods'].append(tags)

    print(f"Found {len(classes)} classes to generate.")
    
    # Write QMD files
    for class_name, class_data in classes.items():
        qmd_filename = f"{class_name.lower()}.qmd"
        qmd_filepath = os.path.join(OUTPUT_DIR, qmd_filename)
        
        # Sort members
        properties = sorted(class_data['properties'], key=lambda x: x['name'])
        methods = sorted(class_data['methods'], key=lambda x: x['name'])
        
        with open(qmd_filepath, 'w', encoding='utf-8') as out:
            out.write(f"---\ntitle: \"Class: {class_name}\"\nsidebar: apidoc\n---\n\n")
            out.write(f"# {class_name} Class\n\n")
            
            # Class Description
            class_desc = convert_indented_code_blocks(replace_crosslinks(class_data['description']))
            out.write(f"{class_desc}\n\n")
            
            # Table of Contents/Overview of members
            if properties:
                out.write("## Properties Overview\n\n")
                out.write("| Name | Type | Description |\n")
                out.write("| :--- | :--- | :--- |\n")
                prop_anchors = {}
                for prop in properties:
                    prop_type = prop.get('type', 'any')
                    # Shorten description for table
                    short_desc = prop.get('description', '').split('\n')[0]
                    short_desc = replace_crosslinks(short_desc)
                    ro = " *(read-only)*" if prop.get('readonly') else ""
                    p_name = prop['name'].lower()
                    if p_name not in prop_anchors:
                        prop_anchors[p_name] = 0
                        anchor = p_name
                    else:
                        prop_anchors[p_name] += 1
                        anchor = f"{p_name}-{prop_anchors[p_name]}"
                    out.write(f"| [`{prop['name']}`](#{anchor}) | `{prop_type}`{ro} | {short_desc} |\n")
                out.write("\n")
                
            if methods:
                out.write("## Methods Overview\n\n")
                out.write("| Method | Return Type | Description |\n")
                out.write("| :--- | :--- | :--- |\n")
                method_anchors = {}
                for method in methods:
                    ret = method.get('return', {})
                    ret_type = ret.get('type', 'void')
                    short_desc = method.get('description', '').split('\n')[0]
                    short_desc = replace_crosslinks(short_desc)
                    # build signature
                    params = method.get('params', [])
                    sig = ", ".join([p['name'] for p in params])
                    m_name = method['name'].lower()
                    if m_name not in method_anchors:
                        method_anchors[m_name] = 0
                        anchor = m_name
                    else:
                        method_anchors[m_name] += 1
                        anchor = f"{m_name}-{method_anchors[m_name]}"
                    out.write(f"| [`{method['name']}({sig})`](#{anchor}) | `{ret_type}` | {short_desc} |\n")
                out.write("\n")
            
            # Detailed Properties
            if properties:
                out.write("## Properties Details\n\n")
                prop_anchors = {}
                for prop in properties:
                    p_name = prop['name'].lower()
                    if p_name not in prop_anchors:
                        prop_anchors[p_name] = 0
                        anchor = p_name
                    else:
                        prop_anchors[p_name] += 1
                        anchor = f"{p_name}-{prop_anchors[p_name]}"
                    out.write(f"### `{prop['name']}` {{#{anchor}}}\n\n")
                    
                    prop_type = prop.get('type', 'any')
                    ro_badge = ' <span class="api-tag tag-readonly">ReadOnly</span>' if prop.get('readonly') else ''
                    out.write(f'<span class="api-tag tag-type">{prop_type}</span>{ro_badge}\n\n')
                    
                    prop_desc = convert_indented_code_blocks(replace_crosslinks(prop.get('description', '')))
                    out.write(f"{prop_desc}\n\n")
                    
                    if 'example' in prop:
                        out.write("```javascript\n")
                        out.write(f"{prop['example']}\n")
                        out.write("```\n\n")
                    out.write("---\n\n")
                    
            # Detailed Methods
            if methods:
                out.write("## Methods Details\n\n")
                method_anchors = {}
                for method in methods:
                    params = method.get('params', [])
                    sig = ", ".join([p['name'] for p in params])
                    
                    m_name = method['name'].lower()
                    if m_name not in method_anchors:
                        method_anchors[m_name] = 0
                        anchor = m_name
                    else:
                        method_anchors[m_name] += 1
                        anchor = f"{m_name}-{method_anchors[m_name]}"
                    out.write(f"### `{method['name']}({sig})` {{#{anchor}}}\n\n")
                    
                    ret = method.get('return', {})
                    ret_type = ret.get('type', 'void')
                    out.write(f"**Returns:** `{ret_type}`\n\n")
                    
                    method_desc = convert_indented_code_blocks(replace_crosslinks(method.get('description', '')))
                    out.write(f"{method_desc}\n\n")
                    
                    if params:
                        out.write("**Parameters:**\n\n")
                        for p in params:
                            out.write(f"*   **`{p['name']}`** (`{p['type']}`): {replace_crosslinks(p['desc'])}\n")
                        out.write("\n")
                        
                    if ret.get('desc'):
                        out.write(f"**Return Value Description:** {replace_crosslinks(ret['desc'])}\n\n")
                        
                    if 'example' in method:
                        out.write("**Example:**\n\n")
                        out.write("```javascript\n")
                        out.write(f"{method['example']}\n")
                        out.write("```\n\n")
                    out.write("---\n\n")
                    
        print(f"Generated: {qmd_filename}")
    
    print("API Documentation generation completed successfully.")

if __name__ == "__main__":
    generate_docs()
