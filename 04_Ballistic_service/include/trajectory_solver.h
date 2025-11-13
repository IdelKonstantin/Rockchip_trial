#ifndef __TRAJECTORY_SOLVER_H__
#define __TRAJECTORY_SOLVER_H__

#include <math.h>
#include "trajectory_solver_API.h"
#include "trajectory_solver_routines.h"
#include "solver_structs_and_consts.h"

void trajectorySolver (const struct Meteo* const meteo, const struct Bullet* const bullet, 
const struct Rifle* const rifle, const struct Scope* const scope, const struct Inputs* const inputs, 
const struct Options* const options, struct Results* OUT results);

#endif /* __TRAJECTORY_SOLVER_H__ */
