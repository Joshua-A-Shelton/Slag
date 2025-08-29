#include "Sampler.h"
#include <slag/backends/Backend.h>
namespace slag
{
    Sampler* Sampler::newSampler(const SamplerParameters& parameters)
    {
        return Backend::current()->newSampler(parameters);
    }
} // slag
