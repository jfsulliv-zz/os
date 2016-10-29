#include <mm/vma.h>
#include <sys/linkerset.h>
#include <sys/kprintf.h>
#include <sys/sysinit.h>
#include <sys/panic.h>
#include <sys/string.h>
#include <util/math.h>

LINKERSET_DEFINE(SYSINIT_LINKERSET, init_step_t);

/* Internal data structure used for topologically sorting the modules by
 * their dependencies. */
typedef struct {
        init_step_t *first_step; // The first step to execute
        init_step_t *last_step; // The last step to execute
        uint64_t dep_mask; // Which modules this module depends on
        uint64_t incoming_dep_mask; // Which modules depend on this one
        bool is_depended_on; // Whether this module is a dependency
} init_module_t;

static bool
depends_on(uint64_t depends_mask, unsigned int module_index)
{
        return depends_mask & (1 << module_index);
}

/* Populates the dependencies for each sysinit module, such that each 
 * module has the dependencies of all of its steps. This prepares the
 * module list for topological sorting.
 * While this is done, we also build up the execution linked-lists of
 * steps for later.
 *
 * Performs basic sanity checks (i.e. that no module depends on itself),
 * leaving more robust cyclicity checks for later. */
static void
collect_init_steps(init_module_t *modules, size_t num)
{
        /* Figure out the depenencies for each module based on its
         * steps. */
        init_step_t **stepp;
        LINKERSET_FOREACH(SYSINIT_LINKERSET, stepp)
        {
                init_step_t *step = *stepp;
                if (!step) {
                        panic("NULL sysinit step");
                }
                unsigned int module_index = LOG2(step->module_mask);
                if (module_index >= num) {
                        kprintf(0, "Init step %s has invalid module %d\n",
                                step->name, step->module_mask);
                        panic("Invalid sysinit step detected.");
                }

                if (depends_on(step->depends_mask, module_index)) {
                        kprintf(0, "Error - init step %s depends on "
                                "its own module.\n", step->name);
                        panic("Trivial dependency cycle detected.");
                }
                if (step->module_mask & ~(1 << module_index)) {
                        kprintf(0, "Error - init step %s has multiple "
                                "modules.\n", step->name);
                        kprintf(0, "Module mask:0x%lx\n",
                                step->module_mask);
                        panic("Ambiguous init step detected.");
                }
                init_module_t *module = &modules[module_index];

                /* Add the dependencies of the step to its module */
                module->dep_mask |= step->depends_mask;

                /* Update the exec linked-list */
                if (!module->first_step) {
                        module->first_step = step;
                } else {
                        module->last_step->_nextptr = step;
                }
                module->last_step = step;
        }
}

/* Populates the reverse dependency masks for each module, so that every
 * module knows which modules depend on it. */
static void
build_reverse_dependencies(init_module_t *modules, size_t num)
{
        unsigned int i = 0;
        for (i = 0; i < num; i++)
        {
                init_module_t *module = modules + i;
                if (!module->first_step) {
                        kprintf(0, "sysinit: Module %d has no steps.\n", i);
                        bug("Empty modules are disallowed.\nRemove the "
                            "module or add an init step.");
                }

                uint64_t dep_mask = module->dep_mask;

                while (dep_mask)
                {
                        unsigned int index = LOG2(dep_mask);

                        init_module_t *dep_module = modules + index;
                        dep_module->incoming_dep_mask |= (1 << i);

                        dep_mask &= ~(1 << index);
                }
        }
}

/* Sort the steps by dependency. Panics if a cycle is detected. */
static void
sort_init_modules(init_module_t *modules, size_t num)
{
        init_module_t *sorted_modules[num];
        init_module_t *start_modules[num];
        bzero(sorted_modules, sizeof(sorted_modules));
        bzero(start_modules, sizeof(start_modules));

        /* Local copy so that we can build up the sorted changes */
        init_module_t copy_modules[num];
        memcpy(copy_modules, modules, sizeof(copy_modules));

        /* Using Kahn's algorithm, sort the modules topologically. */
        size_t num_sorted = 0;
        size_t num_start_modules = 1;
        start_modules[0] = &copy_modules[0];
        while (num_start_modules > 0)
        {
                init_module_t *s = start_modules[--num_start_modules];
                sorted_modules[num_sorted++] = s;

                size_t index_of_s = s - copy_modules;

                uint64_t dep_mask;
                while ((dep_mask = s->incoming_dep_mask) > 0)
                {
                        unsigned int index = LOG2(dep_mask);
                        init_module_t *m = copy_modules + index;

                        /* Remove the edge. */
                        s->incoming_dep_mask &= ~(1 << index);
                        m->dep_mask &= ~(1 << index_of_s);

                        /* If the module now has no incoming
                         * dependencies, we can put it into the
                         * start_modules table */
                        if (!m->dep_mask) {
                                start_modules[num_start_modules++] = m;
                        }
                }
        }
        /* Check for cycles that have not been accounted for. */
        unsigned int i;
        for (i = 0; i < num; i++)
        {
                init_module_t *module = copy_modules + i;
                if (module->dep_mask) {
                        kprintf(0, "sysinit: Cycle detected! Aborting.\n");
                        kprintf(0, "Step %d mask 0x%lx\n", i,
                                module->dep_mask);
                        panic("Unsatisfiable sysinit configuration.");
                }
        }
        /* Everything looks good, so we can now copy the sorted elements
         * back into the argument module list in order. */
        for (i = 0; i < num; i++)
        {
                memcpy(modules + i, sorted_modules[i],
                       sizeof(init_module_t));
        }
}

static int
execute_init_modules(init_module_t *modules, size_t num)
{
        int num_failures = 0;
        unsigned int i = 0;
        for (i = 0; i < num; i++)
        {
                init_module_t *module = &modules[i];
                init_step_t *step = module->first_step;
                while (step)
                {
                        int status = step->step();
                        if (status) {
                                kprintf(0, "sysinit: failed to initialize"
                                        " %s\n", step->name);
                                if (!step->warn_on_fail) {
                                        panic("Critical step failed.");
                                }
                                num_failures++;
                        } else if (i != 0 && i != SYSINIT_NUM_MODULES-1) {
                                kprintf(0, "sysinit: Initialized %s\n",
                                        step->name);
                        }
                        step = step->_nextptr;
                }
        }
        return num_failures;
}

static void
setup_modules(init_module_t *modules)
{
        bzero(modules, sizeof(modules));

        /* Initialize the modules with the EARLY dependency. */
        unsigned int i = 0;
        for (i = 1; i < SYSINIT_NUM_MODULES - 1; i++)
        {
                modules[i].dep_mask = SYSINIT_EARLY;
        }
        /* Also set up the LATE module to have all dependencies. */
        modules[SYSINIT_NUM_MODULES-1].dep_mask = SYSINIT_LATE - 1;

}

/* The list of modules that will be sorted and eventually executed. */
static size_t num_modules = SYSINIT_NUM_MODULES;
static init_module_t modules[SYSINIT_NUM_MODULES];

int
sys_init(void)
{
        /* Initialize the modules with the implicit dependencies. */
        setup_modules(modules);

        /* Collect the init steps into the linked-lists of each module,
         * and also determine the modules' dependencies based on their
         * steps' dependencies. */
        collect_init_steps(modules, num_modules);

        /* Build the backwards dependency masks. */
        build_reverse_dependencies(modules, num_modules);

        /* Sort the modules topologically based on their dependencies.
         * At this point we will find any cycles and panic() on them. */
        sort_init_modules(modules, num_modules);

        /* Actually execute the modules. */
        return execute_init_modules(modules, num_modules);
}
