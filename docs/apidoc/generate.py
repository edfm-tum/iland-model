# iLand JavaScript API Reference Generator
# For instructions on setting up Quarto, local preview, and documentation contribution,
# see the main contributor guide: [docs/README.md](file:///home/werner/dev/iland-model/docs/README.md)

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

def get_category_from_path(file_path):
    if file_path.startswith(SOURCE_API_DIR):
        return 'iland'
    elif file_path.startswith(SOURCE_ABE_DOC_DIR):
        return 'abe'
    elif file_path.startswith(SOURCE_ABE_DIR):
        return 'abe-library'
    else:
        return 'iland'

def replace_crosslinks(text, curr_cat):
    """
    Translates YUI crossLinks to Markdown.
    Example: {{#crossLink "Grid/load:method"}}{{/crossLink}} -> [load()](grid.qmd#load)
    """
    # Replace legacy HTML paths pointing to classes in ABE library module landing page
    text = re.sub(r'href="\.\./classes/(\w+)\.html"', r'href="\1.qmd"', text)

    pattern = re.compile(r'\{\{#crossLink\s+"([^"]+)"\}\}(.*?)\{\{/crossLink\}\}', re.DOTALL)
    def repl(m):
        link_target = m.group(1)
        label = m.group(2).strip()
        
        parts = link_target.split('/')
        class_name = parts[0]
        member_name = None
        member_type = None
        
        if len(parts) > 1:
            member_part = parts[1]
            if ':' in member_part:
                member_name, member_type = member_part.split(':')
            else:
                member_name = member_part
                
        # Handle abe-lib prefix (e.g. abe-lib.helper, abe-lib.harvest.clearcut)
        if class_name.lower().startswith("abe-lib."):
            class_name = class_name[8:] # Strip "abe-lib."
            if '.' in class_name:
                class_name, dot_member = class_name.split('.', 1)
                if not member_name:
                    member_name = dot_member
                    
        class_key = class_name.lower()
        
        class_map = {
            "barkbeetle": "wiki/barkbeetle-module.qmd",
            "wind": "wiki/wind-module.qmd",
            "lib.helper": "abe-library/helper.qmd",
            "mapgrid": "iland/map.qmd",
            "scheduler": "abe/schedule.qmd",
            "patch": "abe/patches.qmd",
            "scriptgrid": "iland/grid.qmd",
        }
        
        target_cat = None
        url = None
        
        if class_key in class_map:
            target_path = class_map[class_key]
            if target_path.startswith("wiki/"):
                target_cat = "wiki"
                url = f"../../../{target_path}" # Going up from apidoc/classes/{curr_cat}/ to docs/
            else:
                target_cat, filename = target_path.split('/')
                url = filename
        else:
            # Look up in parsed symbols (classes and modules)
            # First check classes
            for c_name, c_data in classes.items():
                if c_name.lower() == class_key:
                    target_cat = c_data['category']
                    url = f"{c_name.lower()}.qmd"
                    break
            # Second check modules
            if not target_cat:
                for m_name, m_data in modules.items():
                    if m_name.lower() == class_key:
                        target_cat = m_data['category']
                        url = f"{m_name.lower()}.qmd"
                        break
                        
        if not url:
            # Fallback
            url = f"{class_key}.qmd"
            target_cat = curr_cat
            
        # Adjust URL for different category directory
        if target_cat != "wiki" and target_cat != curr_cat:
            url = f"../{target_cat}/{url}"
            
        if member_name:
            url += f"#{member_name.lower()}"
            
        if not label:
            if member_name:
                if member_type == "method" or "method" in link_target:
                    label = f"{member_name}()"
                else:
                    label = member_name
            else:
                label = class_name
                
        return f"[{label}]({url})"
    return pattern.sub(repl, text)

def convert_indented_code_blocks(text):
    """
    Finds contiguous lines indented by 4 or more spaces or a tab in descriptions
    and wraps them in standard javascript fenced code blocks.
    List items are ignored.
    """
    lines = text.split('\n')
    new_lines = []
    in_block = False
    block_lines = []
    
    for line in lines:
        stripped = line.strip()
        # Avoid treating list items (e.g. - item, * item, 1. item) as code blocks
        is_list = (
            stripped.startswith('- ') or 
            stripped.startswith('* ') or 
            stripped.startswith('+ ') or 
            bool(re.match(r'^\d+\.\s', stripped))
        )
        is_indented = (line.startswith('    ') or line.startswith('\t')) and not is_list
        
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
    
    tags = {}
    
    # Extract tags
    class_match = re.search(r'@class\s+(\w+)', content)
    if class_match:
        tags['class'] = class_match.group(1)
        
    method_match = re.search(r'@method\s+(\w+)', content)
    if method_match:
        tags['method'] = method_match.group(1)
        
    prop_match = re.search(r'@property\s+(\w+)', content)
    if prop_match:
        tags['property'] = prop_match.group(1)
        
    type_match = re.search(r'@type\s+(\w+)', content)
    if type_match:
        tags['type'] = type_match.group(1)
        
    if '@readOnly' in content or '@readonly' in content:
        tags['readonly'] = True
        
    title_match = re.search(r'@title\s+(.*)', content)
    if title_match:
        tags['title'] = title_match.group(1).strip()
        
    module_match = re.search(r'@module\s+(\S+)', content)
    if module_match:
        tags['module'] = module_match.group(1).strip()
        
    # Params tags
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
        
    # Return tag
    return_match = re.search(r'@return\s+\{(\w+)\}\s+(.*?)(?=\n@|\Z)', content, re.DOTALL)
    if return_match:
        tags['return'] = {
            'type': return_match.group(1),
            'desc': return_match.group(2).strip()
        }
        
    # Example tag
    example_match = re.search(r'@Example\s*(.*?)(?=\n@|\Z)', content, re.DOTALL)
    if not example_match:
        example_match = re.search(r'@example\s*(.*?)(?=\n@|\Z)', content, re.DOTALL)
    if example_match:
        tags['example'] = example_match.group(1).strip()
        
    # Extract description
    desc_tag_match = re.search(r'@description\s*(.*?)(?=\n@|\Z)', content, re.DOTALL)
    if desc_tag_match:
        tags['description'] = desc_tag_match.group(1).strip()
    else:
        # Fallback to description before any tag
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

# Global symbols used during link resolution
classes = {}
modules = {}

def generate_docs():
    global classes, modules
    
    # Scan Directories
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
                        
    # Clean up old qmd files in OUTPUT_DIR to prevent stale files
    if os.path.exists(OUTPUT_DIR):
        for f in os.listdir(OUTPUT_DIR):
            if f.endswith('.qmd'):
                os.remove(os.path.join(OUTPUT_DIR, f))
        for sub in ["iland", "abe", "abe-library"]:
            sub_dir = os.path.join(OUTPUT_DIR, sub)
            if os.path.exists(sub_dir):
                for f in os.listdir(sub_dir):
                    if f.endswith('.qmd'):
                        os.remove(os.path.join(sub_dir, f))
                        
    # Ensure nested subdirectories exist
    os.makedirs(os.path.join(OUTPUT_DIR, "iland"), exist_ok=True)
    os.makedirs(os.path.join(OUTPUT_DIR, "abe"), exist_ok=True)
    os.makedirs(os.path.join(OUTPUT_DIR, "abe-library"), exist_ok=True)
    
    classes = {}
    modules = {}
    
    print(f"Parsing {len(source_files)} Javascript files for YUI documentation...")
    
    # Pass 1: Parse and collect all classes and modules
    for file_path in source_files:
        blocks = extract_comments_from_file(file_path)
        category = get_category_from_path(file_path)
        for b in blocks:
            tags = parse_comment_block(b)
            
            # If this is a class header
            if 'class' in tags:
                class_name = tags['class']
                if class_name not in classes:
                    classes[class_name] = {
                        'name': class_name,
                        'description': tags.get('description', ''),
                        'properties': [],
                        'methods': [],
                        'category': category
                    }
                else:
                    if not classes[class_name]['description']:
                        classes[class_name]['description'] = tags.get('description', '')
                        
            # If this is a module header
            elif 'module' in tags:
                module_name = tags['module']
                if module_name not in modules:
                    modules[module_name] = {
                        'name': module_name,
                        'title': tags.get('title', ''),
                        'description': tags.get('description', ''),
                        'category': category
                    }
                else:
                    if not modules[module_name]['description']:
                        modules[module_name]['description'] = tags.get('description', '')
                        
            # If it's a property
            elif 'property' in tags:
                prop_name = tags['property']
                tags['name'] = prop_name
                
                file_blocks = extract_comments_from_file(file_path)
                file_class = None
                for fb in file_blocks:
                    fb_tags = parse_comment_block(fb)
                    if 'class' in fb_tags:
                        file_class = fb_tags['class']
                        break
                
                if file_class:
                    if file_class not in classes:
                        classes[file_class] = {'name': file_class, 'description': '', 'properties': [], 'methods': [], 'category': category}
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
                        classes[file_class] = {'name': file_class, 'description': '', 'properties': [], 'methods': [], 'category': category}
                    classes[file_class]['methods'].append(tags)

    print(f"Found {len(classes)} classes and {len(modules)} modules to generate.")
    
    # Pass 2: Write QMD files with resolved crosslinks
    for class_name, class_data in classes.items():
        category = class_data['category']
        qmd_filename = f"{class_name.lower()}.qmd"
        qmd_filepath = os.path.join(OUTPUT_DIR, category, qmd_filename)
        
        properties = sorted(class_data['properties'], key=lambda x: x['name'])
        methods = sorted(class_data['methods'], key=lambda x: x['name'])
        
        with open(qmd_filepath, 'w', encoding='utf-8') as out:
            out.write(f"---\ntitle: \"Class: {class_name}\"\nsidebar: apidoc\n---\n\n")
            
            class_desc = convert_indented_code_blocks(replace_crosslinks(class_data['description'], category))
            out.write(f"{class_desc}\n\n")
            
            if properties:
                out.write("## Properties Overview\n\n")
                out.write("| Name | Type | Description |\n")
                out.write("| :--- | :--- | :--- |\n")
                prop_anchors = {}
                for prop in properties:
                    prop_type = prop.get('type', 'any')
                    short_desc = prop.get('description', '').split('\n')[0]
                    short_desc = replace_crosslinks(short_desc, category)
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
                    short_desc = replace_crosslinks(short_desc, category)
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
                    
                    prop_desc = convert_indented_code_blocks(replace_crosslinks(prop.get('description', ''), category))
                    out.write(f"{prop_desc}\n\n")
                    
                    if 'example' in prop:
                        out.write("```javascript\n")
                        out.write(f"{prop['example']}\n")
                        out.write("```\n\n")
                    out.write("---\n\n")
                    
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
                    
                    method_desc = convert_indented_code_blocks(replace_crosslinks(method.get('description', ''), category))
                    out.write(f"{method_desc}\n\n")
                    
                    if params:
                        out.write("**Parameters:**\n\n")
                        for p in params:
                            out.write(f"*   **`{p['name']}`** (`{p['type']}`): {replace_crosslinks(p['desc'], category)}\n")
                        out.write("\n")
                        
                    if ret.get('desc'):
                        out.write(f"**Return Value Description:** {replace_crosslinks(ret['desc'], category)}\n\n")
                        
                    if 'example' in method:
                        out.write("**Example:**\n\n")
                        out.write("```javascript\n")
                        out.write(f"{method['example']}\n")
                        out.write("```\n\n")
                    out.write("---\n\n")
                    
        print(f"Generated class: {category}/{qmd_filename}")

    for module_name, module_data in modules.items():
        category = module_data['category']
        qmd_filename = f"{module_name.lower()}.qmd"
        qmd_filepath = os.path.join(OUTPUT_DIR, category, qmd_filename)
        
        with open(qmd_filepath, 'w', encoding='utf-8') as out:
            out.write(f"---\ntitle: \"{module_data['title'] or module_name}\"\nsidebar: apidoc\n---\n\n")
            
            mod_desc = convert_indented_code_blocks(replace_crosslinks(module_data['description'], category))
            out.write(f"{mod_desc}\n\n")
            
        print(f"Generated module: {category}/{qmd_filename}")
    
    print("API Documentation generation completed successfully.")

def generate_bite_docs():
    source_dir = os.path.join(SCRIPT_DIR, "../../src/apidoc/bite")
    output_dir = os.path.join(SCRIPT_DIR, "../bite")
    img_dest_dir = os.path.join(SCRIPT_DIR, "../img")
    
    if not os.path.exists(source_dir):
        print(f"BITE source directory not found at: {source_dir}")
        return
        
    os.makedirs(output_dir, exist_ok=True)
    os.makedirs(img_dest_dir, exist_ok=True)
    
    # Copy images
    source_img_dir = os.path.join(source_dir, "img")
    if os.path.exists(source_img_dir):
        import shutil
        for f in os.listdir(source_img_dir):
            src_file = os.path.join(source_img_dir, f)
            if os.path.isfile(src_file):
                dest_file = os.path.join(img_dest_dir, f)
                shutil.copy2(src_file, dest_file)
                print(f"Copied BITE image {f} to {img_dest_dir}")
                
    # Define mapping of files to title metadata
    titles = {
        "readme.md": "Bite Submodule Overview",
        "variables.md": "Bite Variables, Expressions & Events",
        "codeexamples.md": "Bite Code Examples"
    }
    
    # Process each markdown file
    for f in os.listdir(source_dir):
        if not f.endswith('.md') or f == '_navbar.md':
            continue
            
        src_path = os.path.join(source_dir, f)
        
        # Determine output filename
        if f.lower() == 'readme.md':
            out_name = 'index.qmd'
        else:
            out_name = f.lower().replace('.md', '.qmd')
            
        dest_path = os.path.join(output_dir, out_name)
        
        # Determine title
        f_lower = f.lower()
        if f_lower in titles:
            title = titles[f_lower]
        else:
            class_name = f[:-3] # Strip '.md'
            title = f"{class_name} Class"
            
        with open(src_path, 'r', encoding='utf-8', errors='ignore') as file:
            content = file.read()
            
        # 1. Translate internal links: e.g. [BiteAgent](BiteAgent.md) -> [BiteAgent](biteagent.qmd)
        def link_repl(match):
            label = match.group(1)
            target = match.group(2)
            if target.lower() == 'readme':
                return f"[{label}](index.qmd)"
            else:
                return f"[{label}]({target.lower()}.qmd)"
                
        content = re.sub(r'\[([^\]]+)\]\(([^)]+)\.md\)', link_repl, content)
        
        # 2. Resolve image paths:
        # e.g. ![Bite](img/bite_overview.png ':size=600') -> ![Bite](/img/bite_overview.png){width=600px}
        def img_repl(match):
            alt = match.group(1)
            filename = match.group(2)
            size = match.group(3)
            if size:
                return f"![{alt}](/img/{filename}){{width={size}px}}"
            else:
                return f"![{alt}](/img/{filename})"
                
        content = re.sub(r'!\[([^\]]*)\]\(img/([^\s\)]+)(?:\s+[\'\"]+:size=(\d+)[\'\"])?\)', img_repl, content)
        
        # 3. Fix code fences: convert empty code fences ``` to ```javascript
        lines = content.split('\n')
        new_lines = []
        in_code = False
        for line in lines:
            if line.strip() == '```':
                if not in_code:
                    new_lines.append('```javascript')
                    in_code = True
                else:
                    new_lines.append('```')
                    in_code = False
            else:
                new_lines.append(line)
        content = '\n'.join(new_lines)
        
        # Write file with frontmatter
        with open(dest_path, 'w', encoding='utf-8') as out:
            out.write(f"---\ntitle: \"{title}\"\n---\n\n")
            out.write(content)
            out.write("\n")
            
        print(f"Generated BITE page: {out_name}")


def generate_settings_doc():
    metadata_path = os.path.join(SCRIPT_DIR, "../../src/iland/res/project_file_metadata.txt")
    output_qmd_path = os.path.join(SCRIPT_DIR, "../wiki/project-file-reference.qmd")

    if not os.path.exists(metadata_path):
        print(f"Error: metadata file not found at {metadata_path}")
        return

    with open(metadata_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    c1_titles = {
        "system": "System Settings",
        "model": "Model Settings",
        "output": "Output Settings",
        "modules": "Module Settings",
        "user": "User Settings"
    }

    c2_titles = {
        "system": {
            "path": "File Paths",
            "database": "Databases",
            "settings": "System Settings",
            "logging": "Logging",
            "javascript": "Javascript Options"
        },
        "model": {
            "world": "World & Spatial Setup",
            "site": "Site & Soil Properties",
            "climate": "Climate Configurations",
            "settings": "Model Settings",
            "settings.seedDispersal": "Seed Dispersal",
            "settings.soil": "Soil Settings",
            "settings.grass": "Grass Cover Settings",
            "settings.browsing": "Browsing Settings",
            "settings.permafrost": "Permafrost Settings",
            "settings.svdStates": "SVD States Settings",
            "species": "Species Parameters",
            "initialization": "Initialization Options",
            "management": "Forest Management & ABE",
            "parameter": "Advanced Parameters"
        }
    }

    def map_url(url):
        match = re.match(r'https?://(?:www\.)?iland-model\.org/([^?\s#]+)(?:\?([^#\s]*))?(?:#([^\s]*))?', url)
        if not match:
            return url
        
        page = match.group(1)
        query = match.group(2)
        fragment = match.group(3)
        
        page_clean = page.replace('+', '-').replace('%20', '-').lower()
        
        wiki_dir = os.path.join(SCRIPT_DIR, "../wiki")
        qmd_path = os.path.join(wiki_dir, f"{page_clean}.qmd")
        
        if os.path.exists(qmd_path):
            rel_target = f"{page_clean}.qmd"
            if fragment:
                fragment_clean = fragment.replace('+', '-').replace('%20', '-')
                rel_target += f"#{fragment_clean}"
            
            link_text = page_clean.replace('-', ' ').title()
            return f"[{link_text}]({rel_target})"
        else:
            if page_clean.startswith('bite/'):
                bite_page = page_clean[5:]
                if not bite_page or bite_page == 'index':
                    rel_target = "../bite/index.qmd"
                else:
                    rel_target = f"../bite/{bite_page}.qmd"
                link_text = bite_page.replace('-', ' ').title() if bite_page else "Bite"
                return f"[{link_text}]({rel_target})"
                
            reconstructed = f"https://iland-model.org/{page_clean}"
            if query:
                reconstructed += f"?{query}"
            if fragment:
                reconstructed += f"#{fragment}"
            return f"[{reconstructed}]({reconstructed})"

    def clean_text(text):
        # Strip HTML tags first
        text = re.sub(r'<[^>]+>', ' ', text)
        
        # Format and translate URLs
        def replace_url(match):
            url = match.group(0)
            if "iland-model.org/" in url:
                return map_url(url)
            else:
                return f"[{url}]({url})"
        
        text = re.sub(r'https?://[^\s\)\|]+', replace_url, text)
        # Compress multiple spaces
        text = re.sub(r'\s+', ' ', text).strip()
        return text

    # We will build submodules and track order
    roots = {}  # maps r_name -> list of c2_names in insertion order
    submodules = {}  # maps (r_name, c2_name) -> list of settings
    submodule_descs = {} # maps (r_name, c2_name) -> general description string

    current_layout_desc = ""

    for line in lines:
        line_strip = line.strip()
        if not line_strip or line_strip.startswith(';'):
            continue

        if line_strip.startswith("gui.layout"):
            parts = line_strip.split('=', 1)
            layout_val = parts[1].strip()
            layout_parts = [l.strip() for l in layout_val.split('|')]
            if len(layout_parts) > 0:
                layout_type = layout_parts[0]
                if layout_type in ["tab", "group"]:
                    desc_candidate = ""
                    if layout_type == "tab" and len(layout_parts) > 3:
                        desc_candidate = layout_parts[3]
                    elif layout_type == "group" and len(layout_parts) > 2:
                        desc_candidate = layout_parts[2]
                    elif layout_type == "group" and len(layout_parts) > 1 and not layout_parts[1]:
                        desc_candidate = layout_parts[2] if len(layout_parts) > 2 else ""
                    
                    if desc_candidate:
                        current_layout_desc = desc_candidate
            continue

        if '=' in line_strip:
            parts = line_strip.split('=', 1)
            key = parts[0].strip()
            value_str = parts[1].strip()
            value_parts = [v.strip() for v in value_str.split('|')]

            type_val = value_parts[0] if len(value_parts) > 0 else ""
            
            # Skip connected keys (UI connections only, duplicates of real keys)
            if type_val == "connected":
                continue

            default_val = value_parts[1] if len(value_parts) > 1 else ""
            label_val = value_parts[2] if len(value_parts) > 2 else ""
            tooltip_val = value_parts[3] if len(value_parts) > 3 else ""
            filter_val = value_parts[4] if len(value_parts) > 4 else "simple"

            key_parts = key.split('.')
            if len(key_parts) < 3:
                continue
            
            r_name = key_parts[0]
            if r_name == 'model' and key_parts[1] == 'settings' and key_parts[2] in ['seedDispersal', 'soil', 'grass', 'browsing', 'permafrost', 'svdStates']:
                c2_name = f"settings.{key_parts[2]}"
                rel_key = ".".join(key_parts[3:])
            else:
                c2_name = key_parts[1]
                rel_key = ".".join(key_parts[2:])

            if r_name not in roots:
                roots[r_name] = []
            if c2_name not in roots[r_name]:
                roots[r_name].append(c2_name)

            sub_key = (r_name, c2_name)
            
            # Associate current layout description with the first key's submodule it applies to
            if current_layout_desc:
                if sub_key not in submodule_descs:
                    submodule_descs[sub_key] = clean_text(current_layout_desc)
                current_layout_desc = ""

            if sub_key not in submodules:
                submodules[sub_key] = []

            submodules[sub_key].append({
                'rel_key': rel_key,
                'type': type_val,
                'default': default_val,
                'label': label_val,
                'tooltip': tooltip_val,
                'filter': filter_val
            })

    qmd = []
    qmd.append("---")
    qmd.append("title: \"Project File Settings Reference\"")
    qmd.append("---")
    qmd.append("\n::: {.callout-note}")
    qmd.append("This settings reference is dynamically compiled from the core iLand engine metadata. It displays settings in tabular format organized by submodules.")
    qmd.append(":::\n")
    qmd.append("## Overview\n")
    qmd.append("Project settings are organized by their XML element path hierarchy. The first level represents the root tag (e.g. `system`, `model`), the second level represents the main submodule, and deeper levels represent nested configurations. Settings attributes are formatted with their types, default values, and description tooltips.\n")

    for r_name in roots.keys():
        r_title = c1_titles.get(r_name, r_name.title())
        qmd.append(f"\n# {r_title} (`{r_name}`)\n")
        
        for c2_name in roots[r_name]:
            c2_title = c2_titles.get(r_name, {}).get(c2_name, c2_name.title())
            qmd.append(f"\n## {c2_title} (`{r_name}.{c2_name}`)\n")
            
            sub_key = (r_name, c2_name)
            if sub_key in submodule_descs and submodule_descs[sub_key]:
                qmd.append(f"{submodule_descs[sub_key]}\n")
            
            # Start table
            qmd.append("| **XML Key** | **Type** | **Default Value** | **Description** |")
            qmd.append("| :--- | :--- | :--- | :--- |")
            
            for s in submodules[sub_key]:
                rel_key = s['rel_key']
                t = s['type']
                d = s['default']
                l = s['label']
                
                # Format type
                if t == "file":
                    t_desc = "file selection"
                elif t == "directory":
                    t_desc = "directory selection"
                elif t == "boolean":
                    t_desc = "boolean"
                elif t == "connected":
                    t_desc = "checkbox / toggle"
                elif t == "combo":
                    options = d.split(';')
                    t_desc = f"combo dropdown [options: {', '.join(options)}]"
                    d = options[0] if options else ""
                else:
                    t_desc = t
                
                # Clean description
                desc = clean_text(s['tooltip'])
                
                # Escape pipes
                rel_key_esc = rel_key.replace('|', '\\|')
                t_desc_esc = t_desc.replace('|', '\\|')
                d_esc = d.replace('|', '\\|')
                l_esc = l.strip().replace('|', '\\|')
                desc_esc = desc.strip().replace('|', '\\|')
                
                # Merge Label and Description
                if l_esc and desc_esc:
                    desc_combined = f"**{l_esc}**: {desc_esc}"
                elif l_esc:
                    desc_combined = f"**{l_esc}**"
                else:
                    desc_combined = desc_esc
                
                # Format default value with code block if not empty
                d_formatted = f"`{d_esc}`" if d_esc else ""
                
                qmd.append(f"| `{rel_key_esc}` | {t_desc_esc} | {d_formatted} | {desc_combined} |")
            qmd.append("")

    with open(output_qmd_path, 'w', encoding='utf-8') as f:
        f.write("\n".join(qmd))
        
    print(f"Generated clean tabular reference page at {output_qmd_path}")


if __name__ == "__main__":
    generate_docs()
    generate_bite_docs()
    generate_settings_doc()


