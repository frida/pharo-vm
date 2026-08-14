#ifndef PHAROVM_PHARO_CLIENT_H
#define PHAROVM_PHARO_CLIENT_H

#pragma once

#include "pharovm/parameters/parameters.h"

// CHECK ME: envp is not portable, does it make sense to have it as a parameter here?
VM_EXPORT int vm_main_with_parameters(VMParameters *parameters);
VM_EXPORT int vm_main(int argc, const char **arguments, const char **envp);
VM_EXPORT int vm_init(VMParameters *parameters);
VM_EXPORT void vm_run_interpreter();

#endif //PHAROVM_PHARO_CLIENT_H
