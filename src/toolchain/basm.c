#include "./basm.h"
#include "./bm.h"
#include "./path.h"
#include "./verifier.h"
#include "./target.h"

static char *shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

static void usage(FILE *stream, const char *program)
{
    fprintf(stream, "Usage: %s [OPTIONS] <input.basm>\n", program);
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -I <include/path/>                            Add include path\n");
    fprintf(stream, "    -o <output.bm>                                Provide output path\n");
    fprintf(stream, "    -t <bm|nasm-linux-x86-64|nasm-freebsd-x86-64> Output target. Default is bm\n");
    fprintf(stream, "    -verify                                       Verify the bytecode instructions after the translation\n");
    fprintf(stream, "    -h                                            Print this help to stdout\n");
}

static char *get_flag_value(int *argc, char ***argv,
                            const char *flag,
                            const char *program)
{
    if (*argc == 0) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: no value provided for flag `%s`\n", flag);
        exit(1);
    }

    return shift(argc, argv);
}

int main(int argc, char **argv)
{
    // NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
    static Basm basm = {0};

    const char *program = shift(&argc, &argv);
    const char *input_file_path = NULL;
    const char *output_file_path = NULL;
    Target target = TARGET_BM;
    bool verify = false;

    while (argc > 0) {
        const char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-o") == 0) {
            output_file_path = get_flag_value(&argc, &argv, flag, program);
        } else if (strcmp(flag, "-I") == 0) {
            basm_push_include_path(&basm, sv_from_cstr(get_flag_value(&argc, &argv, flag, program)));
        } else if (strcmp(flag, "-h") == 0) {
            usage(stdout, program);
            exit(0);
        } else if (strcmp(flag, "-t") == 0) {
            const char *name = get_flag_value(&argc, &argv, flag, program);
            if (!target_by_name(name, &target)) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: unknown output format `%s`\n", name);
                exit(1);
            }
        } else if (strcmp(flag, "-verify") == 0) {
            verify = true;
        } else {
            if (input_file_path != NULL) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: input file is already provided as `%s`. Only a single input file is not supported\n", input_file_path);
                exit(1);
            }
            input_file_path = flag;
        }
    }

    if (input_file_path == NULL) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: no input file is provided\n");
        exit(1);
    }

    if (output_file_path == NULL) {
        const String_View output_file_path_sv =
            SV_CONCAT(&basm.arena,
                      SV("./"),
                      file_name_of_path(input_file_path),
                      target_file_ext(target));
        output_file_path = arena_sv_to_cstr(&basm.arena, output_file_path_sv);
    }

    basm_translate_root_source_file(&basm, sv_from_cstr(input_file_path));

    if (verify) {
        static Verifier verifier = {0};
        verifier_verify(&verifier, &basm);
    }

    switch (target) {
    case TARGET_BM: {
        basm_save_to_file_as_bm(&basm, output_file_path);
    }
    break;

    case TARGET_NASM_LINUX_X86_64: {
        basm_save_to_file_as_nasm_sysv_x86_64(&basm, SYSCALLTARGET_LINUX, output_file_path);
    }
    break;

    case TARGET_NASM_FREEBSD_X86_64: {
        basm_save_to_file_as_nasm_sysv_x86_64(&basm, SYSCALLTARGET_FREEBSD, output_file_path);
    }
    break;

    case COUNT_TARGETS:
    default:
        assert(false && "basm: unreachable");
        exit(1);
    }

    arena_free(&basm.arena);

    return 0;
}
