#pragma once

#if RPM_ESTIMATOR_TYPE == RPM_ESTIMATOR_LINEAR
    #include "LinearRpmEstimator.h"
    using RpmEstimator = LinearRpmEstimator;
#elif RPM_ESTIMATOR_TYPE == RPM_ESTIMATOR_POTENTIOMETER
    #include "PotRpmEstimator.h"
    using RpmEstimator = PotRpmEstimator;
#elif RPM_ESTIMATOR_TYPE == RPM_ESTIMATOR_HALL
    #include "HallRpmEstimator.h"
    using RpmEstimator = HallRpmEstimator;
#endif
