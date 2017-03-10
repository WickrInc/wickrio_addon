#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "cspec_output_verbose.h"
#include "test_util.h"

#include "cspec_output_unit.h"

static const char *ci_flag = "--ci";

int main(int argc, char *argv[])
{
    bool CI_MODE = false;
    
    if (argc > 1) {
        if (strcmp(argv[1], ci_flag) == 0) {
            CI_MODE = true;
        }
    }
    
    CSpecOutputStruct* output = CI_MODE ? CSpec_NewOutputUnit() : CSpec_NewOutputVerbose();
    
    CSpec_Run(DESCRIPTION(getBase64FromData), output);
    
    return output->failed;
}
