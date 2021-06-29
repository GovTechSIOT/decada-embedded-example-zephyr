Import("env", "projenv")

from elftools.elf.elffile import ELFFile


def find_offset(sym, elf: ELFFile):

    for seg in elf.iter_segments():
        if seg.header.p_type != "PT_LOAD":
            continue

        # ELF file offset
        if (
            sym["st_value"] >= seg["p_vaddr"]
            and sym["st_value"] < seg["p_vaddr"] + seg["p_filesz"]
        ):
            return sym["st_value"] - seg["p_vaddr"] + seg["p_offset"]

    return None


def get_vector_table(elf: ELFFile):

    symtab = elf.get_section_by_name(".symtab")
    if symtab is None:
        print("Failed to find .symtab section")
        return None

    vector_start_sym = symtab.get_symbol_by_name("_vector_start")[0]
    if vector_start_sym is None:
        print("Failed to find _vector_start symbol")
        return None

    vector_end_sym = symtab.get_symbol_by_name("_vector_end")[0]
    if vector_end_sym is None:
        print("Failed to find _vector_end symbol")
        return None

    start_offset = find_offset(vector_start_sym, elf)
    end_offset = find_offset(vector_end_sym, elf)

    if start_offset and end_offset:
        print(
            "Found vector table offsets: {} - {}".format(
                hex(start_offset), hex(end_offset)
            )
        )
        elf.stream.seek(start_offset)

        return {
            "data": elf.stream.read(end_offset - start_offset),
            "offset": start_offset,
        }


def copy_vector_table(source, target, env):

    # Get values from build flags
    BUILD_FLAGS = {
        k: v for (k, v) in [x for x in env["CPPDEFINES"] if type(x) == tuple]
    }
    ROM_OFFSET = int(BUILD_FLAGS["COPY_VT_ROM_OFFSET"], 16)
    NVS_OFFSET = int(BUILD_FLAGS["COPY_VT_NVS_OFFSET"], 16)
    NVS_LENGTH = int(BUILD_FLAGS["COPY_VT_NVS_LENGTH"], 16)

    elf = ELFFile(open(target[0].get_abspath(), "rb+"))

    vector_table = get_vector_table(elf)
    if vector_table is None:
        exit(1)

    vector_table_data, vector_table_offset = (
        vector_table["data"],
        vector_table["offset"],
    )
    vector_table_len = len(vector_table_data)
    base_offset = vector_table_offset - ROM_OFFSET
    print("ROM_OFFSET = ", hex(ROM_OFFSET))

    # Should be overwriting only null bytes
    elf.stream.seek(base_offset)
    if elf.stream.read(vector_table_len) != b"\x00" * vector_table_len:
        print("Overwriting non-NULL bytes - Is CONFIG_ROM_START_OFFSET set?")

    elf.stream.seek(base_offset)
    elf.stream.write(vector_table_data)

    print("Copied vector table")

    elf.stream.seek(base_offset + NVS_OFFSET)
    elf.stream.write(b"\xFF" * NVS_LENGTH)

    print(
        "Cleared flash partition (offset: {}; length: {})".format(
            NVS_OFFSET, NVS_LENGTH
        )
    )


env.AddPostAction("$BUILD_DIR/firmware.elf", copy_vector_table)
