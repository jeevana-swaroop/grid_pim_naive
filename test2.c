#include <mram.h>
#include <alloc.h>
#include <stdint.h>
#include "parm.h"
#include <stdio.h>
#include <defs.h>
#include <mutex.h>
#include <barrier.h>

__mram indexStructure structureArr[2048];
__mram float arrangedData[2048 * DIM];
__mram uint64_t myIndex;
__host uint64_t structSize;
__host uint64_t dataSize;
__host uint64_t iDIMLengths[iDIM];
__host uint64_t startGrid;
__host uint64_t endGrid;
__host uint64_t count = 0;

MUTEX_INIT(my_mutex);
BARRIER_INIT(my_barrier, NR_TASKLETS);

// int linearSearch(__mram_ptr indexStructure *structure, uint64_t *location, uint64_t key)
// {

//     for (uint64_t i = 0; i < structSize; i++)
//     {

//         if (structure[i].linearCoordinate == key)
//         {

//             *location = i;
//             return 0;
//         }
//     }
//     return -1;
// }

int binary_search(__mram_ptr indexStructure *structure, uint64_t size, uint64_t key, uint64_t *location)
{

    int64_t left = 0;
    int64_t right = size - 1;
    int64_t mid;

    while (left <= right)
    {
        mid = (right + left) / 2;

        if (structure[mid].linearCoordinate == key)
        {
            *location = (uint64_t)mid;
            return 0;
        }
        else if (structure[mid].linearCoordinate < key)
        {
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }
    return -1;
}

int pow_int(int x, int y)
{
    int answ = 1;
    for (int i = 0; i < y; i++)
    {
        answ *= x;
    }
    return answ;
}

uint64_t getLinearID_nDimensionsGPU(int64_t *indexes, uint64_t pointIndex,
                                    uint64_t *dimLen, uint64_t nDimensions)
{
    uint64_t offset = 0;
    uint64_t multiplier = 1;
    for (uint64_t i = 0; i < nDimensions; i++)
    {
        offset += ((uint64_t)(indexes[pointIndex * nDimensions + i])) * multiplier;
        multiplier *= dimLen[i];
    }

    return offset;
}

int euDist(__mram_ptr float *dataset, uint64_t qPtIndex, uint64_t cPtIndex)
{

    float sqDist = 0;
    float epsilon_sq = (EPSILON) * (EPSILON);
    for (uint64_t i = 0; i < DIM; i++)
    {

        sqDist += ((dataset[qPtIndex * DIM + i] - dataset[cPtIndex * DIM + i]) *
                   (dataset[qPtIndex * DIM + i] - dataset[cPtIndex * DIM + i]));

        if (epsilon_sq < sqDist)
        {
            return 0;
        }
    }
    return 1;
}

int main()
{
    uint64_t mytaskId = me();
    uint64_t dataStartPoint;
    uint64_t dataCount;
    int check = 0;
    int64_t temp[iDIM];
    uint64_t queryLinearId;
    uint64_t queryDataStartPoint;
    uint64_t queryCount;
    uint64_t queryLocation;
    int res;
    uint64_t localCount = 0;

    // printf("%ld-%ld-%ld-%ld-%ld-%ld-%ld\n", myIndex, structSize, dataSize, iDIMLengths[0], iDIMLengths[1], startGrid, endGrid);
    //  if (mytaskId == 0)
    //  {
    for (int i = mytaskId; i < structSize; i += NR_TASKLETS)
    {
        // printf("%ld-%ld-%ld-%ld-%ld-%ld\n", structureArr[i].linearCoordinate, structureArr[i].gridCoordinates[0], structureArr[i].gridCoordinates[1], structureArr[i].startIndex, structureArr[i].endIndex, structureArr[i].count);

        if (structureArr[i].gridCoordinates[0] >= startGrid && structureArr[i].gridCoordinates[0] < endGrid)
        {
            // uint64_t qid[9] = {0};

            // linearSearch(structureArr, &dataStartPoint, &dataCount, structureArr[i].linearCoordinate);

            for (int j = 0; j < pow_int(3, iDIM); j++)
            {

                for (int k = 0; k < iDIM; k++)
                {

                    int op = (j / pow_int(3, k)) % 3;

                    if (op == 0)
                    {
                        temp[k] = (int64_t)(structureArr[i].gridCoordinates[k]) + 0;
                    }
                    else if (op == 1)
                    {

                        temp[k] = (int64_t)(structureArr[i].gridCoordinates[k]) + 1;
                    }
                    else
                    {
                        temp[k] = (int64_t)(structureArr[i].gridCoordinates[k]) - 1;
                    }

                    if (temp[k] < 0)
                    {
                        check = 1;
                        break;
                    }
                }

                if (check == 1)
                {
                    check = 0;
                    continue;
                }
                queryLinearId = getLinearID_nDimensionsGPU(temp, 0, iDIMLengths, iDIM);

                res = binary_search(structureArr, structSize, queryLinearId, &queryLocation);

                if (res == 0)
                {
                    // qid[j] = queryLinearId;
                    //  printf("%ld-->%ld\n", structureArr[i].linearCoordinate, queryLinearId);
                    //  printf("%ld, ", queryLinearId);
                    for (uint64_t q = structureArr[i].startIndex; q <= structureArr[i].endIndex; q++)
                    {

                        for (uint64_t c = structureArr[queryLocation].startIndex; c <= structureArr[queryLocation].endIndex; c++)
                        {
                            if (euDist(arrangedData, q, c))
                            {
                                localCount++;
                                mutex_lock(my_mutex);
                                count++;
                                mutex_unlock(my_mutex);
                            }
                        }
                    }
                }

                check = 0;
                queryDataStartPoint = 0;
                queryCount = 0;
            }
            // printf("%ld-%ld-%ld-%ld-%ld-%ld-%ld-%ld-%ld-%ld\n", qid[0], qid[1], qid[2], qid[3], qid[4], qid[5], qid[6], qid[7], qid[8], localCount);

            // printf("%ld-%ld-%ld-%ld-%ld\n", structureArr[i].linearCoordinate, structureArr[i].gridCoordinates[0], structureArr[i].gridCoordinates[1], mytaskId, localCount);
        }
        localCount = 0;
        dataStartPoint = 0;
        dataCount = 0;
    }
    barrier_wait(&my_barrier);
    if (mytaskId == 0)
        printf("%lu\n", count);
    //}
    return 0;
}