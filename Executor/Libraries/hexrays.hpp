#include "../Executor.h"

bool decompile_function(ea_t func_addr, std::string& out_pseudocode)
{
    if (!init_hexrays_plugin()) {
        msg("Hex-Rays decompiler is not available.\n");
        return false;
    }
    func_t* func = get_func(func_addr);
    hexrays_failure_t hf;

    msg("Func: %llx\n", func);
    if (func == nullptr)
    {
        return false;
    }
    cfuncptr_t cfunc = decompile(func, &hf, DECOMP_WARNINGS);
    if (cfunc == nullptr) {
        msg("Failed to decompile function at address: 0x%X\n", func_addr);
        return false;
    }
    out_pseudocode = "";
    const strvec_t& sv = cfunc->get_pseudocode();
    for (int i = 0; i < sv.size(); i++) {
        qstring buf;
        tag_remove(&buf, sv[i].line);
        //msg("%s\n", buf.c_str());
        out_pseudocode += buf.c_str();
        out_pseudocode += "\n";
    }
    //out_pseudocode = "gay";
    return true;

}

namespace LUDA::Library
{
    static int c_decompile(lua_State* L)
    {
        ea_t addr = (ea_t)lua_tointeger(L, 1);

        // Check if address is in a function
        if (!get_func(addr)) {
            lua_pushnil(L);
            lua_pushstring(L, "Address is not in a function");
            return 2;
        }

        std::string pseudocode = "no pseudo";
        if (decompile_function(addr, pseudocode)) {
            lua_pushstring(L, pseudocode.c_str());
            return 1;
        }
        else {
            lua_pushnil(L);
            lua_pushstring(L, "Decompilation failed");
            return 2;
        }
    }

    static int c_disassemble(lua_State* L) {
        ea_t func_addr = lua_tointeger(L, 1);
        //msg("Looking for function at: 0x%X\n", func_addr);
        func_t* func = get_func(func_addr);
        if (!func) {
            //msg("ERROR: get_func returned NULL\n");
            lua_pushnil(L);
            return 1;
        }
        //msg("Function found: start=0x%X, end=0x%X\n", func->start_ea, func->end_ea);
        if (func->end_ea == 0 || func->end_ea <= func->start_ea) {
            //msg("ERROR: Invalid function bounds\n");
            lua_pushnil(L);
            return 1;
        }
        lua_newtable(L);
        int table_idx = 1;
        for (ea_t addr = func->start_ea; addr < func->end_ea; ) {
            insn_t insn;
            int size = decode_insn(&insn, addr);
            if (size == 0) {
                msg("Failed to decode at 0x%X\n", addr);
                addr++;
                continue;
            }
            /* Create instruction table */
            lua_newtable(L);
            qstring qstr;
            qstring buf;
            generate_disasm_line(&qstr, addr);
            tag_remove(&buf, qstr);
            lua_pushstring(L, buf.c_str());
            lua_setfield(L, -2, "disasm");
            char mnem[64] = { 0 };
            sscanf(buf.c_str(), "%63s", mnem);
            lua_pushstring(L, mnem);
            lua_setfield(L, -2, "op");
            lua_pushinteger(L, insn.ea);
            lua_setfield(L, -2, "ea");
            lua_pushinteger(L, insn.size);
            lua_setfield(L, -2, "size");

            /* Operands table */
            lua_newtable(L);
            int op_idx = 1;
            for (int i = 0; i < UA_MAXOP; i++) {
                op_t& op = insn.ops[i];
                if (op.type == o_void) break;

                lua_newtable(L);

                /* Operand type */
                lua_pushinteger(L, op.type);
                lua_setfield(L, -2, "type");

                /* Type name */
                const char* type_name = "unknown";
                switch (op.type) {
                case o_reg:     type_name = "reg"; break;
                case o_mem:     type_name = "mem"; break;
                case o_phrase:  type_name = "phrase"; break;
                case o_displ:   type_name = "displ"; break;
                case o_imm:     type_name = "imm"; break;
                case o_far:     type_name = "far"; break;
                case o_near:    type_name = "near"; break;
                }
                lua_pushstring(L, type_name);
                lua_setfield(L, -2, "type_name");

                /* Register name (if register operand) */
                if (op.type == o_reg) {
                    qstring reg_buf;
                    size_t width = op.dtype ? get_dtype_size(op.dtype) : 8;
                    get_reg_name(&reg_buf, op.reg, width);
                    lua_pushstring(L, reg_buf.c_str());
                    lua_setfield(L, -2, "reg_name");
                }

                /* Value */
                lua_pushinteger(L, op.value);
                lua_setfield(L, -2, "value");

                /* Offset/displacement */
                lua_pushinteger(L, op.addr);
                lua_setfield(L, -2, "addr");

                lua_rawseti(L, -2, op_idx);
                op_idx++;
            }
            lua_setfield(L, -2, "operands");

            /* Registers used table */
            lua_newtable(L);
            int reg_idx = 1;
            for (int i = 0; i < UA_MAXOP; i++) {
                op_t& op = insn.ops[i];
                if (op.type == o_void) break;

                if (op.type == o_reg) {
                    qstring reg_buf;
                    size_t width = op.dtype ? get_dtype_size(op.dtype) : 8;
                    get_reg_name(&reg_buf, op.reg, width);
                    lua_pushstring(L, reg_buf.c_str());
                    lua_rawseti(L, -2, reg_idx);
                    reg_idx++;
                }
            }
            lua_setfield(L, -2, "regs");

            /* Features/flags - check mnemonic */
            lua_newtable(L);

            bool is_jump = strstr(mnem, "j") != NULL && strcmp(mnem, "mov") != 0;
            bool is_call = strcmp(mnem, "call") == 0;
            bool is_ret = strcmp(mnem, "ret") == 0 || strcmp(mnem, "retn") == 0;

            lua_pushboolean(L, is_jump);
            lua_setfield(L, -2, "is_jump");
            lua_pushboolean(L, is_call);
            lua_setfield(L, -2, "is_call");
            lua_pushboolean(L, is_ret);
            lua_setfield(L, -2, "is_ret");

            lua_setfield(L, -2, "flags");

            lua_rawseti(L, -2, table_idx);
            table_idx++;
            addr += insn.size;
        }
        return 1;
    }
}