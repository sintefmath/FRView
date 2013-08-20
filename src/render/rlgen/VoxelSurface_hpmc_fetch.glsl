/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

#ifndef VOXEL_SAMPLER_DEFINED
uniform usampler3D voxels;
#endif

float
HPMC_fetch( vec3 p )
{
    return float(texture( voxels, p ).r > 0u );
}
