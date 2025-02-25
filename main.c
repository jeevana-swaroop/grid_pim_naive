
#include "utility.h"
#include <dpu.h>
#include <assert.h>

int main(int argc, char **argv)
{

  uint64_t N = 1000;
  float epsilon = (float)EPSILON;

  float *dataset = (float *)malloc(sizeof(float) * N * DIM);

  float *maxValues = (float *)malloc(sizeof(float) * iDIM);
  float *minValues = (float *)malloc(sizeof(float) * iDIM);

  uint64_t *iDIMLengths = (uint64_t *)malloc(sizeof(uint64_t) * iDIM);

  uint64_t *gridCoordinates = (uint64_t *)malloc(sizeof(uint64_t) * N * iDIM);

  pointNId *pointNIdArr = (pointNId *)malloc(sizeof(pointNId) * N);

  char inputFname[] = "iono_57min_5.16Mpts_2D.txt";

  int ret = importDataset(inputFname, N, dataset, DIM);

  if (ret == 1)
  {
    return 0;
  }

  findMaxMin(dataset, maxValues, minValues, iDIM, DIM, N);

  for (uint64_t i = 0; i < iDIM; i++)
  {
    iDIMLengths[i] = (uint64_t)((maxValues[i] - minValues[i]) / epsilon) + 1;
  }

  for (uint64_t i = 0; i < N; i++)
  {
    findGridCoordinate(gridCoordinates, pointNIdArr, dataset, i, maxValues,
                       minValues, &epsilon, iDIM, DIM, iDIMLengths);
  }

  // for (int i = 0; i < N; i++)
  // {

  //   printf("%ld==>", pointNIdArr[i].linearCoordinate);

  //   for (int j = 0; j < iDIM; j++)
  //   {
  //     printf(" %ld ", gridCoordinates[i * iDIM + j]);
  //   }
  //   printf("\n");
  // }

  qsort(pointNIdArr, N, sizeof(pointNId), compare);

  uint64_t gridsCount = 1;

  for (uint64_t i = 1; i < N; i++)
  {

    if (pointNIdArr[i].linearCoordinate !=
        pointNIdArr[i - 1].linearCoordinate)
    {
      gridsCount++;
    }
  }

  printf("%ld\n", gridsCount);

  uint64_t *uniqueGridCoordinates =
      (uint64_t *)malloc(sizeof(uint64_t) * gridsCount);
  indexStructure *indStructure =
      (indexStructure *)malloc(sizeof(indexStructure) * gridsCount);

  uint64_t start = 0;
  uint64_t end = 0;

  for (uint64_t i = 1, j = 0; i < N && j < gridsCount; i++)
  {

    if (pointNIdArr[i].linearCoordinate !=
        pointNIdArr[i - 1].linearCoordinate)
    {

      uniqueGridCoordinates[j] = pointNIdArr[i - 1].linearCoordinate;
      indStructure[j].linearCoordinate = pointNIdArr[i - 1].linearCoordinate;
      indStructure[j].startIndex = start;
      indStructure[j].endIndex = end;
      indStructure[j].count = (end - start) + 1;

      for (int k = 0; k < iDIM; k++)
      {

        indStructure[j].gridCoordinates[k] = gridCoordinates[pointNIdArr[i - 1].pointIndex * iDIM + k];
      }

      start = i;

      j++;
    }

    if (i == N - 1)
    {

      end = N - 1;
      uniqueGridCoordinates[j] = pointNIdArr[start].linearCoordinate;
      indStructure[j].linearCoordinate = pointNIdArr[start].linearCoordinate;
      indStructure[j].startIndex = start;
      indStructure[j].endIndex = end;
      indStructure[j].count = (end - start) + 1;

      for (int k = 0; k < iDIM; k++)
      {

        indStructure[j].gridCoordinates[k] = gridCoordinates[pointNIdArr[start].pointIndex * iDIM + k];
      }
    }
    end++;
  }

  qsort(indStructure, gridsCount, sizeof(indexStructure), compare_1);

  for (int i = 0; i < gridsCount; i++)
  {

    printf("%ld-->%ld-->%ld\n", indStructure[i].linearCoordinate, indStructure[i].gridCoordinates[0], indStructure[i].gridCoordinates[1]);
  }

  // for (int i = 0; i < gridsCount; i++)
  // {

  //   printf("%ld--%ld--%ld--%ld--%ld-->%ld\n", indStructure[i].gridCoordinates[0], indStructure[i].gridCoordinates[1], indStructure[i].linearCoordinate, indStructure[i].startIndex, indStructure[i].endIndex, indStructure[i].count);
  // }

  // uint64_t max_points = 0;

  // for (int i = 0; i < gridsCount; i++)
  // {

  //   if (indStructure[i].count > max_points)
  //   {
  //     max_points = indStructure[i].count;
  //   }
  // }

  // printf("\n%ld", max_points);

  float *arrangedDataset = (float *)malloc(sizeof(float) * N * DIM);
  uint64_t ind = 0;
  for (int i = 0; i < gridsCount; i++)
  {

    for (int k = indStructure[i].startIndex; k <= indStructure[i].endIndex; k++)
    {

      for (int d = 0; d < DIM; d++)
      {

        arrangedDataset[ind * DIM + d] = dataset[pointNIdArr[k].pointIndex * DIM + d];
      }

      ind++;
    }
  }
  printf("\n%ld\n", ind);

  // for (int i = 0; i < N; i++)
  // {

  //   for (int j = 0; j < DIM; j++)
  //   {

  //     printf("%f ", arrangedDataset[i * DIM + j]);
  //   }
  //   printf("\n");
  // }

  struct dpu_set_t dpu_set, dpu;

  DPU_ASSERT(dpu_alloc(NR_DPUS, "backend=simulator", &dpu_set));

  DPU_ASSERT(dpu_load(dpu_set, "./test2", NULL));

  uint64_t dpuIdx;
  DPU_FOREACH(dpu_set, dpu, dpuIdx)
  {

    uint64_t load = (uint64_t)(iDIMLengths[0] / NR_DPUS);

    printf("\n%ld-->%ld\n", iDIMLengths[0], load);

    uint64_t startGrid = dpuIdx * load;
    uint64_t endGrid = startGrid + load;

    if (dpuIdx == NR_DPUS - 1)
    {
      endGrid = iDIMLengths[0];
    }
    printf("\n%ld-->%ld\n", startGrid, endGrid);

    int64_t gridStartPoint = -1;
    uint64_t gridArrSize = 0;
    uint64_t dataStartPoint = 0;
    uint64_t dataArrSize = 0;

    uint64_t pointsInGridStart = 0;

    printf("gridcount = %ld\n", gridsCount);
    for (int i = 0; i < gridsCount; i++)
    {

      if (indStructure[i].gridCoordinates[0] > endGrid)
      {

        break;
      }
      else if ((int64_t)(indStructure[i].gridCoordinates[0]) >= (int64_t)(startGrid)-1 && indStructure[i].gridCoordinates[0] <= endGrid)
      {

        if (gridStartPoint == -1)
        {
          gridStartPoint = i;
        }
        gridArrSize++;
        dataArrSize += indStructure[i].count;

        indStructure[i].startIndex = pointsInGridStart;
        indStructure[i].endIndex = pointsInGridStart + indStructure[i].count - 1;
        pointsInGridStart += indStructure[i].count;
      }
      else
      {
        dataStartPoint += indStructure[i].count;
      }
    }

    indexStructure *tempBuffer = (indexStructure *)malloc(sizeof(indexStructure) * gridArrSize);

    memcpy(tempBuffer, indStructure + gridStartPoint, sizeof(indexStructure) * gridArrSize);

    qsort(tempBuffer, gridArrSize, sizeof(indexStructure), compare_2);

    printf("%ld-->%ld-->%ld-->%ld\n", gridStartPoint, gridArrSize, dataStartPoint, dataArrSize);
    printf("%ld-->%ld\n", sizeof(indexStructure) * gridArrSize, dpuIdx);

    uint64_t adjustedSize = dataArrSize + (dataArrSize % 8);

    DPU_ASSERT(dpu_copy_to(dpu, "myIndex", 0, &dpuIdx, sizeof(uint64_t)));
    DPU_ASSERT(dpu_copy_to(dpu, "structureArr", 0, tempBuffer, sizeof(indexStructure) * gridArrSize));
    DPU_ASSERT(dpu_copy_to(dpu, "arrangedData", 0, arrangedDataset + (dataStartPoint * DIM), sizeof(float) * DIM * adjustedSize));
    DPU_ASSERT(dpu_copy_to(dpu, "structSize", 0, &gridArrSize, sizeof(uint64_t)));
    DPU_ASSERT(dpu_copy_to(dpu, "dataSize", 0, &dataArrSize, sizeof(uint64_t)));
    DPU_ASSERT(dpu_copy_to(dpu, "iDIMLengths", 0, iDIMLengths, sizeof(uint64_t) * iDIM));
    DPU_ASSERT(dpu_copy_to(dpu, "startGrid", 0, &startGrid, sizeof(uint64_t)));
    DPU_ASSERT(dpu_copy_to(dpu, "endGrid", 0, &endGrid, sizeof(uint64_t)));
  }

  dpu_launch(dpu_set, DPU_SYNCHRONOUS);

  uint64_t *count = (uint64_t *)calloc(NR_DPUS, sizeof(uint64_t));
  uint64_t globalCount = 0;

  DPU_FOREACH(dpu_set, dpu, dpuIdx)
  {
    DPU_ASSERT(dpu_log_read(dpu, stdout));

    DPU_ASSERT(dpu_copy_from(dpu, "count", 0, count + dpuIdx, sizeof(uint64_t)));
  }

  for (int i = 0; i < NR_DPUS; i++)
  {

    globalCount += count[i];
  }

  printf("\ncount: %ld\n", globalCount);

  return 0;
}
