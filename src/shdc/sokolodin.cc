/*
    Generate sokol-odin module.
*/
#include "shdc.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc {

using namespace util;

static std::string file_content;

#if defined(_MSC_VER)
#define L(str, ...) file_content.append(fmt::format(str, __VA_ARGS__))
#else
#define L(str, ...) file_content.append(fmt::format(str, ##__VA_ARGS__))
#endif

static const char* uniform_type_to_sokol_type_str(uniform_t::type_t type) {
    switch (type) {
        case uniform_t::FLOAT:  return ".FLOAT";
        case uniform_t::FLOAT2: return ".FLOAT2";
        case uniform_t::FLOAT3: return ".FLOAT3";
        case uniform_t::FLOAT4: return ".FLOAT4";
        case uniform_t::INT:    return ".INT";
        case uniform_t::INT2:   return ".INT2";
        case uniform_t::INT3:   return ".INT3";
        case uniform_t::INT4:   return ".INT4";
        case uniform_t::MAT4:   return ".MAT4";
        default: return "FIXME";
    }
}

static const char* uniform_type_to_flattened_sokol_type_str(uniform_t::type_t type) {
    switch (type) {
        case uniform_t::FLOAT:
        case uniform_t::FLOAT2:
        case uniform_t::FLOAT3:
        case uniform_t::FLOAT4:
        case uniform_t::MAT4:
             return ".FLOAT4";
        case uniform_t::INT:
        case uniform_t::INT2:
        case uniform_t::INT3:
        case uniform_t::INT4:
            return ".INT4";
        default: return "FIXME";
    }
}

static const char* img_type_to_sokol_type_str(image_t::type_t type) {
    switch (type) {
        case image_t::IMAGE_TYPE_2D:    return "._2D";
        case image_t::IMAGE_TYPE_CUBE:  return ".CUBE";
        case image_t::IMAGE_TYPE_3D:    return "._3D";
        case image_t::IMAGE_TYPE_ARRAY: return ".ARRAY";
        default: return "INVALID";
    }
}

static const char* img_basetype_to_sokol_sampletype_str(image_t::sampletype_t sampletype) {
    switch (sampletype) {
        case image_t::IMAGE_SAMPLETYPE_FLOAT: return ".FLOAT";
        case image_t::IMAGE_SAMPLETYPE_DEPTH: return ".DEPTH";
        case image_t::IMAGE_SAMPLETYPE_SINT:  return ".SINT";
        case image_t::IMAGE_SAMPLETYPE_UINT:  return ".UINT";
        default: return "INVALID";
    }
}

static const char* smp_type_to_sokol_type_str(sampler_t::type_t type) {
    switch (type) {
        case sampler_t::SAMPLER_TYPE_SAMPLE: return ".SAMPLE";
        case sampler_t::SAMPLER_TYPE_COMPARE: return ".COMPARE";
        default: return "INVALID";
    }
}

static const char* sokol_backend(slang_t::type_t slang) {
    switch (slang) {
        case slang_t::GLSL330:      return ".GLCORE33";
        case slang_t::GLSL100:      return ".GLES3";
        case slang_t::GLSL300ES:    return ".GLES3";
        case slang_t::HLSL4:        return ".D3D11";
        case slang_t::HLSL5:        return ".D3D11";
        case slang_t::METAL_MACOS:  return ".METAL_MACOS";
        case slang_t::METAL_IOS:    return ".METAL_IOS";
        case slang_t::METAL_SIM:    return ".METAL_SIMULATOR";
        case slang_t::WGSL:         return ".WGPU";
        default: return "<INVALID>";
    }
}

static void write_header(const args_t& args, const input_t& inp, const spirvcross_t& spirvcross) {
    L("/*\n");
    L("    #version:{}# (machine generated, don't edit!)\n", args.gen_version);
    L("\n");
    L("    Generated by sokol-shdc (https://github.com/floooh/sokol-tools)\n");
    L("\n");
    L("    Cmdline: {}\n", args.cmdline);
    L("\n");
    L("    Overview:\n");
    L("\n");
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;

        const spirvcross_source_t* vs_src = find_spirvcross_source_by_shader_name(prog.vs_name, inp, spirvcross);
        const spirvcross_source_t* fs_src = find_spirvcross_source_by_shader_name(prog.fs_name, inp, spirvcross);
        assert(vs_src && fs_src);
        L("        Shader program '{}':\n", prog.name);
        L("            Get shader desc: shd.{}{}_shader_desc(sg.query_backend());\n", mod_prefix(inp), prog.name);
        L("            Vertex shader: {}\n", prog.vs_name);
        L("                Attribute slots:\n");
        const snippet_t& vs_snippet = inp.snippets[vs_src->snippet_index];
        for (const attr_t& attr: vs_src->refl.inputs) {
            if (attr.slot >= 0) {
                L("                    ATTR_{}{}_{} = {}\n", mod_prefix(inp), vs_snippet.name, attr.name, attr.slot);
            }
        }
        for (const uniform_block_t& ub: vs_src->refl.uniform_blocks) {
            L("                Uniform block '{}':\n", ub.struct_name);
            L("                    C struct: {}{}_t\n", mod_prefix(inp), ub.struct_name);
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), ub.struct_name, ub.slot);
        }
        for (const image_t& img: vs_src->refl.images) {
            L("                Image '{}':\n", img.name);
            L("                    Image Type: {}\n", img_type_to_sokol_type_str(img.type));
            L("                    Sample Type: {}\n", img_basetype_to_sokol_sampletype_str(img.sample_type));
            L("                    Multisampled: {}\n", img.multisampled);
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), img.name, img.slot);
        }
        for (const sampler_t& smp: vs_src->refl.samplers) {
            L("                Sampler '{}':\n", smp.name);
            L("                    Type: {}\n", smp_type_to_sokol_type_str(smp.type));
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), smp.name, smp.slot);
        }
        for (const image_sampler_t& img_smp: vs_src->refl.image_samplers) {
            L("                Image Sampler Pair '{}':\n", img_smp.name);
            L("                    Image: {}\n", img_smp.image_name);
            L("                    Sampler: {}\n", img_smp.sampler_name);
        }
        L("            Fragment shader: {}\n", prog.fs_name);
        for (const uniform_block_t& ub: fs_src->refl.uniform_blocks) {
            L("                Uniform block '{}':\n", ub.struct_name);
            L("                    C struct: {}{}_t\n", mod_prefix(inp), ub.struct_name);
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), ub.struct_name, ub.slot);
        }
        for (const image_t& img: fs_src->refl.images) {
            L("                Image '{}':\n", img.name);
            L("                    Image Type: {}\n", img_type_to_sokol_type_str(img.type));
            L("                    Sample Type: {}\n", img_basetype_to_sokol_sampletype_str(img.sample_type));
            L("                    Multisampled: {}\n", img.multisampled);
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), img.name, img.slot);
        }
        for (const sampler_t& smp: fs_src->refl.samplers) {
            L("                Sampler '{}':\n", smp.name);
            L("                    Type: {}\n", smp_type_to_sokol_type_str(smp.type));
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), smp.name, smp.slot);
        }
        for (const image_sampler_t& img_smp: fs_src->refl.image_samplers) {
            L("                Image Sampler Pair '{}':\n", img_smp.name);
            L("                    Image: {}\n", img_smp.image_name);
            L("                    Sampler: {}\n", img_smp.sampler_name);
        }
        L("\n");
    }
    L("*/\n");
    for (const auto& header: inp.headers) {
        L("{}\n", header);
    }
}

static void write_vertex_attrs(const input_t& inp, const spirvcross_t& spirvcross) {
    for (const spirvcross_source_t& src: spirvcross.sources) {
        if (src.refl.stage == stage_t::VS) {
            const snippet_t& vs_snippet = inp.snippets[src.snippet_index];
            for (const attr_t& attr: src.refl.inputs) {
                if (attr.slot >= 0) {
                    L("ATTR_{}{}_{} :: {}\n", mod_prefix(inp), vs_snippet.name, attr.name, attr.slot);
                }
            }
        }
    }
}

static void write_image_bind_slots(const input_t& inp, const spirvcross_t& spirvcross) {
    for (const image_t& img: spirvcross.unique_images) {
        L("SLOT_{}{} :: {}\n", mod_prefix(inp), img.name, img.slot);
    }
}

static void write_sampler_bind_slots(const input_t& inp, const spirvcross_t& spirvcross) {
    for (const sampler_t& smp: spirvcross.unique_samplers) {
        L("SLOT_{}{} :: {}\n", mod_prefix(inp), smp.name, smp.slot);
    }
}

static void write_uniform_blocks(const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang) {
    for (const uniform_block_t& ub: spirvcross.unique_uniform_blocks) {
        L("SLOT_{}{} :: {}\n", mod_prefix(inp), ub.struct_name, ub.slot);
        L("{} :: struct {{\n", to_ada_case(fmt::format("{}{}", mod_prefix(inp), ub.struct_name)));
        int cur_offset = 0;
        for (const uniform_t& uniform: ub.uniforms) {
            int next_offset = uniform.offset;
            if (next_offset > cur_offset) {
                L("    _: [{}]u8,\n", next_offset - cur_offset);
                cur_offset = next_offset;
            }
            if (inp.ctype_map.count(uniform_type_str(uniform.type)) > 0) {
                // user-provided type names
                if (uniform.array_count == 1) {
                    L("    {}: {},\n", uniform.name, inp.ctype_map.at(uniform_type_str(uniform.type)));
                }
                else {
                    L("    {}: [{}]{},\n", uniform.name, uniform.array_count, inp.ctype_map.at(uniform_type_str(uniform.type)));
                }
            }
            else {
                // default type names (float)
                if (uniform.array_count == 1) {
                    switch (uniform.type) {
                        case uniform_t::FLOAT:   L("    {}: f32,\n", uniform.name); break;
                        case uniform_t::FLOAT2:  L("    {}: [2]f32,\n", uniform.name); break;
                        case uniform_t::FLOAT3:  L("    {}: [3]f32,\n", uniform.name); break;
                        case uniform_t::FLOAT4:  L("    {}: [4]f32,\n", uniform.name); break;
                        case uniform_t::INT:     L("    {}: i32,\n", uniform.name); break;
                        case uniform_t::INT2:    L("    {}: [2]i32,\n", uniform.name); break;
                        case uniform_t::INT3:    L("    {}: [3]i32,\n", uniform.name); break;
                        case uniform_t::INT4:    L("    {}: [4]i32,\n", uniform.name); break;
                        case uniform_t::MAT4:    L("    {}: [16]f32,\n", uniform.name); break;
                        default:                 L("    INVALID_UNIFORM_TYPE,\n"); break;
                    }
                }
                else {
                    switch (uniform.type) {
                        case uniform_t::FLOAT4:  L("    {}: [{}][4]f32,\n", uniform.name, uniform.array_count); break;
                        case uniform_t::INT4:    L("    {}: [{}][4]i32,\n", uniform.name, uniform.array_count); break;
                        case uniform_t::MAT4:    L("    {}: [{}][16]f32,\n", uniform.name, uniform.array_count); break;
                        default:                 L("    INVALID_UNIFORM_TYPE,\n"); break;
                    }
                }
            }
            cur_offset += uniform_size(uniform.type, uniform.array_count);
        }
        // pad to multiple of 16-bytes struct size
        const int round16 = roundup(cur_offset, 16);
        if (cur_offset != round16) {
            L("    _: [{}]u8,\n", round16-cur_offset);
        }
        L("}}\n");
    }
}

static void write_shader_sources_and_blobs(const input_t& inp,
                                           const spirvcross_t& spirvcross,
                                           const bytecode_t& bytecode,
                                           slang_t::type_t slang)
{
    for (int snippet_index = 0; snippet_index < (int)inp.snippets.size(); snippet_index++) {
        const snippet_t& snippet = inp.snippets[snippet_index];
        if ((snippet.type != snippet_t::VS) && (snippet.type != snippet_t::FS)) {
            continue;
        }
        int src_index = spirvcross.find_source_by_snippet_index(snippet_index);
        assert(src_index >= 0);
        const spirvcross_source_t& src = spirvcross.sources[src_index];
        int blob_index = bytecode.find_blob_by_snippet_index(snippet_index);
        const bytecode_blob_t* blob = 0;
        if (blob_index != -1) {
            blob = &bytecode.blobs[blob_index];
        }
        std::vector<std::string> lines;
        pystring::splitlines(src.source_code, lines);
        // first write the source code in a comment block
        L("/*\n");
        for (const std::string& line: lines) {
            L("   {}\n", util::replace_C_comment_tokens(line));
        }
        L("*/\n");
        if (blob) {
            std::string c_name = fmt::format("{}{}_bytecode_{}", mod_prefix(inp), snippet.name, slang_t::to_str(slang));
            L("@(private)\n");
            L("{} := [{}]u8 {{\n", c_name.c_str(), blob->data.size());
            const size_t len = blob->data.size();
            for (size_t i = 0; i < len; i++) {
                if ((i & 15) == 0) {
                    L("    ");
                }
                L("{:#04x},", blob->data[i]);
                if ((i & 15) == 15) {
                    L("\n");
                }
            }
            L("\n}}\n");
        }
        else {
            // if no bytecode exists, write the source code, but also a byte array with a trailing 0
            std::string c_name = fmt::format("{}{}_source_{}", mod_prefix(inp), snippet.name, slang_t::to_str(slang));
            const size_t len = src.source_code.length() + 1;
            L("@(private)\n");
            L("{} := [{}]u8 {{\n", c_name.c_str(), len);
            for (size_t i = 0; i < len; i++) {
                if ((i & 15) == 0) {
                    L("    ");
                }
                L("{:#04x},", src.source_code[i]);
                if ((i & 15) == 15) {
                    L("\n");
                }
            }
            L("\n}}\n");
        }
    }
}


static void write_stage(const char* indent,
                        const char* stage_name,
                        const spirvcross_source_t* src,
                        const std::string& src_name,
                        const bytecode_blob_t* blob,
                        const std::string& blob_name,
                        slang_t::type_t slang)
{
    if (blob) {
        L("{}desc.{}.bytecode.ptr = &{};\n", indent, stage_name, blob_name);
        L("{}desc.{}.bytecode.size = {};\n", indent, stage_name, blob->data.size());
    }
    else {
        L("{}desc.{}.source = transmute(cstring)&{}\n", indent, stage_name, src_name);
        const char* d3d11_tgt = nullptr;
        if (slang == slang_t::HLSL4) {
            d3d11_tgt = (0 == strcmp("vs", stage_name)) ? "vs_4_0" : "ps_4_0";
        }
        else if (slang == slang_t::HLSL5) {
            d3d11_tgt = (0 == strcmp("vs", stage_name)) ? "vs_5_0" : "ps_5_0";
        }
        if (d3d11_tgt) {
            L("{}desc.{}.d3d11_target = \"{}\"\n", indent, stage_name, d3d11_tgt);
        }
    }
    assert(src);
    L("{}desc.{}.entry = \"{}\"\n", indent, stage_name, src->refl.entry_point);
    for (int ub_index = 0; ub_index < uniform_block_t::NUM; ub_index++) {
        const uniform_block_t* ub = find_uniform_block_by_slot(src->refl, ub_index);
        if (ub) {
            L("{}desc.{}.uniform_blocks[{}].size = {}\n", indent, stage_name, ub_index, roundup(ub->size, 16));
            L("{}desc.{}.uniform_blocks[{}].layout = .STD140\n", indent, stage_name, ub_index);
            if (slang_t::is_glsl(slang) && (ub->uniforms.size() > 0)) {
                if (ub->flattened) {
                    L("{}desc.{}.uniform_blocks[{}].uniforms[0].name = \"{}\"\n", indent, stage_name, ub_index, ub->struct_name);
                    L("{}desc.{}.uniform_blocks[{}].uniforms[0].type = {};\n", indent, stage_name, ub_index, uniform_type_to_flattened_sokol_type_str(ub->uniforms[0].type));
                    L("{}desc.{}.uniform_blocks[{}].uniforms[0].array_count = {}\n", indent, stage_name, ub_index, roundup(ub->size, 16) / 16);
                }
                else {
                    for (int u_index = 0; u_index < (int)ub->uniforms.size(); u_index++) {
                        const uniform_t& u = ub->uniforms[u_index];
                        L("{}desc.{}.uniform_blocks[{}].uniforms[{}].name = \"{}.{}\"\n", indent, stage_name, ub_index, u_index, ub->inst_name, u.name);
                        L("{}desc.{}.uniform_blocks[{}].uniforms[{}].type = {}\n", indent, stage_name, ub_index, u_index, uniform_type_to_sokol_type_str(u.type));
                        L("{}desc.{}.uniform_blocks[{}].uniforms[{}].array_count = {}\n", indent, stage_name, ub_index, u_index, u.array_count);
                    }
                }
            }
        }
    }
    for (int img_index = 0; img_index < image_t::NUM; img_index++) {
        const image_t* img = find_image_by_slot(src->refl, img_index);
        if (img) {
            L("{}desc.{}.images[{}].used = true\n", indent, stage_name, img_index);
            L("{}desc.{}.images[{}].multisampled = {}\n", indent, stage_name, img_index, img->multisampled ? "true" : "false");
            L("{}desc.{}.images[{}].image_type = {}\n", indent, stage_name, img_index, img_type_to_sokol_type_str(img->type));
            L("{}desc.{}.images[{}].sample_type = {}\n", indent, stage_name, img_index, img_basetype_to_sokol_sampletype_str(img->sample_type));
        }
    }
    for (int smp_index = 0; smp_index < sampler_t::NUM; smp_index++) {
        const sampler_t* smp = find_sampler_by_slot(src->refl, smp_index);
        if (smp) {
            L("{}desc.{}.samplers[{}].used = true\n", indent, stage_name, smp_index);
            L("{}desc.{}.samplers[{}].sampler_type = {}\n", indent, stage_name, smp_index, smp_type_to_sokol_type_str(smp->type));
        }
    }
    for (int img_smp_index = 0; img_smp_index < image_sampler_t::NUM; img_smp_index++) {
        const image_sampler_t* img_smp = find_image_sampler_by_slot(src->refl, img_smp_index);
        if (img_smp) {
            L("{}desc.{}.image_sampler_pairs[{}].used = true\n", indent, stage_name, img_smp_index);
            L("{}desc.{}.image_sampler_pairs[{}].image_slot = {}\n", indent, stage_name, img_smp_index, find_image_by_name(src->refl, img_smp->image_name)->slot);
            L("{}desc.{}.image_sampler_pairs[{}].sampler_slot = {}\n", indent, stage_name, img_smp_index, find_sampler_by_name(src->refl, img_smp->sampler_name)->slot);
            if (slang_t::is_glsl(slang)) {
                L("{}desc.{}.image_sampler_pairs[{}].glsl_name = \"{}\"\n", indent, stage_name, img_smp_index, img_smp->name);
            }
        }
    }
}

static void write_shader_desc_init(const char* indent, const program_t& prog, const input_t& inp, const spirvcross_t& spirvcross, const bytecode_t& bytecode, slang_t::type_t slang) {
    const spirvcross_source_t* vs_src = find_spirvcross_source_by_shader_name(prog.vs_name, inp, spirvcross);
    const spirvcross_source_t* fs_src = find_spirvcross_source_by_shader_name(prog.fs_name, inp, spirvcross);
    assert(vs_src && fs_src);
    const bytecode_blob_t* vs_blob = find_bytecode_blob_by_shader_name(prog.vs_name, inp, bytecode);
    const bytecode_blob_t* fs_blob = find_bytecode_blob_by_shader_name(prog.fs_name, inp, bytecode);
    std::string vs_src_name, fs_src_name;
    std::string vs_blob_name, fs_blob_name;
    if (vs_blob) {
        vs_blob_name = fmt::format("{}{}_bytecode_{}", mod_prefix(inp), prog.vs_name, slang_t::to_str(slang));
    }
    else {
        vs_src_name = fmt::format("{}{}_source_{}", mod_prefix(inp), prog.vs_name, slang_t::to_str(slang));
    }
    if (fs_blob) {
        fs_blob_name = fmt::format("{}{}_bytecode_{}", mod_prefix(inp), prog.fs_name, slang_t::to_str(slang));
    }
    else {
        fs_src_name = fmt::format("{}{}_source_{}", mod_prefix(inp), prog.fs_name, slang_t::to_str(slang));
    }

    /* write shader desc */
    for (int attr_index = 0; attr_index < attr_t::NUM; attr_index++) {
        const attr_t& attr = vs_src->refl.inputs[attr_index];
        if (attr.slot >= 0) {
            if (slang_t::is_glsl(slang)) {
                L("{}desc.attrs[{}].name = \"{}\"\n", indent, attr_index, attr.name);
            }
            else if (slang_t::is_hlsl(slang)) {
                L("{}desc.attrs[{}].sem_name = \"{}\"\n", indent, attr_index, attr.sem_name);
                L("{}desc.attrs[{}].sem_index = {}\n", indent, attr_index, attr.sem_index);
            }
        }
    }
    write_stage(indent, "vs", vs_src, vs_src_name, vs_blob, vs_blob_name, slang);
    write_stage(indent, "fs", fs_src, fs_src_name, fs_blob, fs_blob_name, slang);
    L("{}desc.label = \"{}{}_shader\"\n", indent, mod_prefix(inp), prog.name);
}

errmsg_t sokolodin_t::gen(const args_t& args, const input_t& inp,
                     const std::array<spirvcross_t,slang_t::NUM>& spirvcross,
                     const std::array<bytecode_t,slang_t::NUM>& bytecode)
{
    // first write everything into a string, and only when no errors occur,
    // dump this into a file (so we don't have half-written files lying around)
    file_content.clear();

    bool comment_header_written = false;
    bool common_decls_written = false;
    for (int i = 0; i < slang_t::NUM; i++) {
        slang_t::type_t slang = (slang_t::type_t) i;
        if (args.slang & slang_t::bit(slang)) {
            errmsg_t err = check_errors(inp, spirvcross[i], slang);
            if (err.valid) {
                return err;
            }
            if (!comment_header_written) {
                write_header(args, inp, spirvcross[i]);
                comment_header_written = true;
            }
            if (!common_decls_written) {
                common_decls_written = true;
                write_vertex_attrs(inp, spirvcross[i]);
                write_image_bind_slots(inp, spirvcross[i]);
                write_sampler_bind_slots(inp, spirvcross[i]);
                write_uniform_blocks(inp, spirvcross[i], slang);
            }
            write_shader_sources_and_blobs(inp, spirvcross[i], bytecode[i], slang);
        }
    }

    // write access functions which return sg.ShaderDesc structs
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;
        L("{}{}_shader_desc :: proc (backend: sg.Backend) -> sg.Shader_Desc {{\n", mod_prefix(inp), prog.name);
        L("    desc: sg.Shader_Desc\n");
        L("    #partial switch backend {{\n");
        for (int i = 0; i < slang_t::NUM; i++) {
            slang_t::type_t slang = (slang_t::type_t) i;
            if (args.slang & slang_t::bit(slang)) {
                L("        case {}: {{\n", sokol_backend(slang));
                write_shader_desc_init("            ", prog, inp, spirvcross[i], bytecode[i], slang);
                L("        }}\n");
            }
        }
        L("    }}\n");
        L("    return desc\n");
        L("}}\n");
    }

    // write result into output file
    FILE* f = fopen(args.output.c_str(), "w");
    if (!f) {
        return errmsg_t::error(inp.base_path, 0, fmt::format("failed to open output file '{}'", args.output));
    }
    fwrite(file_content.c_str(), file_content.length(), 1, f);
    fclose(f);
    return errmsg_t();
}

} // namespace shdc
