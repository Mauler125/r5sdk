//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef DETOURCROWDINTERNAL_H
#define DETOURCROWDINTERNAL_H

void integrate(dtCrowdAgent* ag, const float dt);
bool overOffmeshConnection(const dtCrowdAgent* ag, const float radius);
float getDistanceToGoal(const dtCrowdAgent* ag, const float range);
void calcSmoothSteerDirection(const dtCrowdAgent* ag, float* dir);
void calcStraightSteerDirection(const dtCrowdAgent* ag, float* dir);
int addNeighbour(const int idx, const float dist,
	dtCrowdNeighbour* neis, const int nneis, const int maxNeis);
int getNeighbours(const float* pos, const float height, const float range,
	const dtCrowdAgent* skip, dtCrowdNeighbour* result, const int maxResult,
	dtCrowdAgent** agents, const int /*nagents*/, dtProximityGrid* grid);
int addToOptQueue(dtCrowdAgent* newag, dtCrowdAgent** agents, const int nagents, const int maxAgents);
int addToPathQueue(dtCrowdAgent* newag, dtCrowdAgent** agents, const int nagents, const int maxAgents);

#endif // DETOURCROWDINTERNAL_H
