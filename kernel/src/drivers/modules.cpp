#include "modules.h"
#include "graphics/BasicRenderer.h"

extern "C" void modules_init() {
    int err;
    for(module_t* module = &_ModulesStart; module != &_ModulesEnd; module++) {
        switch(module->module_type) {
            case MODULE_TYPE_GENERAL:
                err = ((module_general_t *)module->module)->init_func();
                break;
            case MODULE_TYPE_PCI:
                continue;
            default:
                GlobalRenderer->Printf("Modules: %s of type %d not implemented\n", 
                    module->module_name, module->module_type);
        }

        if(err) {
            GlobalRenderer->Printf("Modules: %s failed initialization with error %d\n",
                module->module_name, err);
        }
    }
}