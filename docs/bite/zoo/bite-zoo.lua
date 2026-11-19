-- bite-zoo.lua
-- Single-source .js code inclusion (with download link) and paper-url normalization for BITE Zoo pages.

local function html_escape(s)
  return (s:gsub("&", "&amp;")
           :gsub("<", "&lt;")
           :gsub(">", "&gt;")
           :gsub('"', "&quot;")
           :gsub("'", "&#39;"))
end

local function get_doc_dir()
  local input_file = nil
  if quarto and quarto.doc and quarto.doc.input_file then
    input_file = quarto.doc.input_file
  elseif PANDOC_STATE and PANDOC_STATE.input_files and PANDOC_STATE.input_files[1] then
    input_file = PANDOC_STATE.input_files[1]
  end
  if not input_file then
    return "."
  end
  local dir = pandoc.path.directory(input_file)
  if dir == "" then
    return "."
  end
  return dir
end

local function expand_code_file(el)
  local fname = el.attributes["file"]
  if not fname or fname == "" then
    return nil
  end

  local doc_dir = get_doc_dir()
  local fullpath = pandoc.path.join({doc_dir, fname})
  local f, err = io.open(fullpath, "r")
  if not f then
    error("bite-zoo.lua: could not open code file '" .. fullpath .. "': " .. tostring(err))
  end
  local content = f:read("*a")
  f:close()

  -- Strip a single trailing newline so code block rendering stays crisp
  content = content:gsub("\r?\n$", "")

  el.text = content
  el.attributes["file"] = nil
  if not el.attributes["filename"] then
    el.attributes["filename"] = fname
  end

  local esc = html_escape(fname)
  local dl = pandoc.RawBlock("html",
    '<div class="bite-zoo-download">' ..
    '<a href="' .. esc .. '" download="' .. esc .. '" class="btn btn-sm btn-outline-primary">' ..
    '<i class="bi bi-download"></i> Download <code>' .. esc .. '</code></a></div>'
  )

  return { dl, el }
end

local function normalize_meta(meta)
  if meta["paper-doi"] then
    local doi = pandoc.utils.stringify(meta["paper-doi"])
    if doi ~= "" then
      local href = doi
      if not href:match("^https?://") then
        href = "https://doi.org/" .. doi
      end
      meta["paper-url"] = pandoc.MetaString(href)
      return meta
    end
  end
  return nil
end

return {
  { Meta = normalize_meta },
  { CodeBlock = expand_code_file }
}
