#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define Fan_Perimeter_Ratio 1850.0
#define Gas_efficiency 8.6
#define mileage
#define InitTemp 30
#define InitCoolingLevel 2
#define Min 60.0
#define InitGas 80

//프로세스 통신 관련
#define TEMP_KEY (key_t)60120
#define RPM_KEY (key_t)60121
#define GAS_KEY (key_t)60122
#define COOLANT_KEY (key_t)60123
#define INPUT_TEMP_KEY (key_t)60124
#define COOL_KEY (key_t)60125

//표시될 data들 프로세스간 통신이 될때 사용됨
int tempid;
int rpmid;
int gasid;
int coolantid;
int inputTempid;
int coolid;
void *temp = (void *)0;
void *rpm = (void *)0;
void *gas = (void *)0;
void *coolant = (void *)0;
void *inputTemp = (void *)0;
void *cool = (void *)0;
int *tempVal = 0;
int *rpmVal = 0;
int *gasVal = 0;
int *coolantVal = 0;
int *inputTempVal = 0;
int *coolVal = 0;

//동기화를 위한 tool
pthread_mutex_t mut;
pthread_mutex_t rpmut;
pthread_cond_t cmd;

//프로세스 통신 관련 초기화
void init_shm()
{
  if ((tempid = shmget(TEMP_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0)
    exit(4);
  if ((rpmid = shmget(RPM_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0)
    exit(4);
  if ((gasid = shmget(GAS_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0)
    exit(4);
  if ((coolantid = shmget(COOLANT_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0)
    exit(4);
  if ((inputTempid = shmget(INPUT_TEMP_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0)
    exit(4);
  if ((coolid = shmget(COOL_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0)
    exit(4);

  if ((temp = shmat(tempid, 0, 0)) < 0)
    exit(5);
  if ((rpm = shmat(rpmid, 0, 0)) < 0)
    exit(5);
  if ((gas = shmat(gasid, 0, 0)) < 0)
    exit(5);
  if ((coolant = shmat(coolantid, 0, 0)) < 0)
    exit(5);
  if ((inputTemp = shmat(inputTempid, 0, 0)) < 0)
    exit(5);
  if ((cool = shmat(coolid, 0, 0)) < 0)
    exit(5);

  tempVal = (int *)temp;
  rpmVal = (int *)rpm;
  gasVal = (int *)gas;
  coolantVal = (int *)coolant;
  inputTempVal = (int *)inputTemp;
  coolVal = (int *)cool;
}
// 온도값을 계산하는 함수
// 초기에 온도값과 Cooling Level값을 다른 프로세스에서 받아와 계산함
void *Figures_Temp()
{
  while (1)
  {
    pthread_mutex_lock(&mut);
    if (*tempVal < 5)
    {
      *tempVal += *coolVal;
    }
    else if (*tempVal >= 5)
    {
      *tempVal -= *coolVal;
    }
    pthread_cond_signal(&cmd);
    pthread_mutex_unlock(&mut);
    //printf("Speed\t: %.2lf (km/h)\n", *tempVal);
    usleep(1000000);
  }
}
// RPM 값을 계산하는 함수
// 스레드로 계산되는 온도값을 읽어와 순간 RPM을 계산하는 함수
// RPM = 온도 * 10 * 1000000 / 60분 / Fan의 지름
void *Figures_RPM()
{
  while (1)
  {
    // pthread_mutex_lock(&mut);
    pthread_mutex_lock(&rpmut);
    *rpmVal = *tempVal * 10 * 1000000.0 / Min / Fan_Perimeter_Ratio;
    //  pthread_cond_wait(&cmd, &mut);
    pthread_mutex_unlock(&rpmut);
    usleep(1000000);
    //pthread_mutex_unlock(&mut);

    //printf("RPM\t: %.2lf (RPM)\n", *rpmVal);
  }
}
// 가스의 양을 계산하는 함수
// 소모량 = 가스소모량/8.7
void *Figures_Gas()
{
  *gasVal = 80;
  while (1)
  {

    double result;
    //  pthread_mutex_lock(&mut);

    result = *rpmVal * Fan_Perimeter_Ratio / 1000000.0;
    // pthread_cond_wait(&cmd, &mut);

    //  pthread_mutex_unlock(&mut);
    *gasVal = *gasVal - (result / 1000) / (8.7);
    usleep(1000000);

    //printf("gas\t: %.2lf (L)\n", *gasVal);
  }
}
// 냉각수 온도
// 실제로는 센서로 측정하지만
// 연관 관계를 나타내기위헤 온도에 맞춰서 계산함
void *Figures_Coolant()
{
  while (1)
  {

    //pthread_mutex_lock(&mut);
    if (*tempVal == 0.0)
    {
      *coolantVal = 0.0;
    }
    else if (*tempVal > 0 && *tempVal <= 8)
    {
      *coolantVal = 0.0;
    }
    else if (*tempVal > 8 && *tempVal <= 14)
    {
      *coolantVal = 5.0;
    }
    else if (*tempVal > 14 && *tempVal <= 16)
    {
      *coolantVal = 10.0;
    }
    else if (*tempVal > 16 && *tempVal < 20)
    {
      *coolantVal = 15.0;
    }
    usleep(1000000);
    //pthread_cond_wait(&cmd,&mut);
    //pthread_mutex_unlock(&mut);
    //printf("Coolant\t: %.2lf (℃  )\n", *coolantVal);
  }
}
int main()
{
  int re = 0;
  init_shm();
  pthread_mutex_init(&mut, NULL);
  pthread_mutex_init(&rpmut, NULL);
  pthread_cond_init(&cmd, NULL);
  pthread_t threads[4];

  re = pthread_create(&threads[0], NULL, Figures_Temp, NULL);
  if (re < 0)
  {
    //printf("threads[0] Figures_Temp error\n");
    exit(0);
  }
  re = pthread_create(&threads[1], NULL, Figures_RPM, NULL);
  if (re < 0)
  {
    //printf("threads[1] Figures_Temp error\n");
    exit(0);
  }
  re = pthread_create(&threads[2], NULL, Figures_Gas, NULL);
  if (re < 0)
  {
    //printf("threads[2] Figures_Temp error\n");
    exit(0);
  }
  re = pthread_create(&threads[3], NULL, Figures_Coolant, NULL);
  if (re < 0)
  {
    //printf("threads[3] Figures_Temp error\n");
    exit(0);
  }

  sleep(5000);
  pthread_mutex_destroy(&mut);
  pthread_cond_destroy(&cmd);
  pthread_mutex_destroy(&rpmut);
  return 0;
}