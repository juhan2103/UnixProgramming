#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define Tire_Perimeter_Ratio 1850.0
#define Fuel_efficiency 8.6
#define mileage
#define InitSpeed 30
#define Initacceleration 2
#define Min 60.0
#define InitFeul 80

//프로세스 통신 관련
#define SPEED_KEY (key_t)60110
#define RPM_KEY (key_t)60111
#define FUEL_KEY (key_t)60112
#define COOLANT_KEY (key_t)60113
#define INPUT_SPEED_KEY (key_t)60114
#define ACC_KEY (key_t)60115

//표시될 data들 프로세스간 통신이 될때 사용됨
int speedid;
int rpmid;
int fuelid;
int coolantid;
int inputSpeedid;
int accid;
void *speed = (void *)0;
void *rpm = (void *)0;
void *fuel = (void *)0;
void *coolant = (void *)0;
void *inputSpeed = (void *)0;
void *acc = (void *)0;
int *speedVal = 0;
int *rpmVal = 0;
int *fuelVal = 0;
int *coolantVal = 0;
int *inputSpeedVal = 0;
int *accVal = 0;

//동기화를 위한 tool
pthread_mutex_t mut;
pthread_mutex_t rpmut;
pthread_cond_t cmd;

//프로세스 통신 관련 초기화
void init_shm()
{
  if ((speedid = shmget(SPEED_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0)
    exit(4);
  if ((rpmid = shmget(RPM_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0)
    exit(4);
  if ((fuelid = shmget(FUEL_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0)
    exit(4);
  if ((coolantid = shmget(COOLANT_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0)
    exit(4);
  if ((inputSpeedid = shmget(INPUT_SPEED_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0)
    exit(4);
  if ((accid = shmget(ACC_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0)
    exit(4);

  if ((speed = shmat(speedid, 0, 0)) < 0)
    exit(5);
  if ((rpm = shmat(rpmid, 0, 0)) < 0)
    exit(5);
  if ((fuel = shmat(fuelid, 0, 0)) < 0)
    exit(5);
  if ((coolant = shmat(coolantid, 0, 0)) < 0)
    exit(5);
  if ((inputSpeed = shmat(inputSpeedid, 0, 0)) < 0)
    exit(5);
  if ((acc = shmat(accid, 0, 0)) < 0)
    exit(5);

  speedVal = (int *)speed;
  rpmVal = (int *)rpm;
  fuelVal = (int *)fuel;
  coolantVal = (int *)coolant;
  inputSpeedVal = (int *)inputSpeed;
  accVal = (int *)acc;
}
// 속도값을 계산하는 함수
// 초기에 속력값과 가속도값을 다른 프로세스에서 받아와 계산함
void *Figures_Speed()
{
  while (1)
  {
    pthread_mutex_lock(&mut);
    if (*speedVal < 30)
    {
      *speedVal += *accVal;
    }
    else if (*speedVal >= 30)
    {
      *speedVal -= *accVal;
    }
    pthread_cond_signal(&cmd);
    pthread_mutex_unlock(&mut);
    //printf("Speed\t: %.2lf (km/h)\n", *speedVal);
    usleep(1000000);
  }
}
// RPM 값을 계산하는 함수
// 스레드로 계산되는 속도값을 읽어와 순간 RPM을 계산하는 함수
// RPM = 속도 * 1000000 / 60분 / 타이어의 지름
void *Figures_RPM()
{
  while (1)
  {
    // pthread_mutex_lock(&mut);
    pthread_mutex_lock(&rpmut);
    *rpmVal = *speedVal * 1000000.0 / Min / Tire_Perimeter_Ratio;
    //  pthread_cond_wait(&cmd, &mut);
    pthread_mutex_unlock(&rpmut);
    usleep(1000000);
    //pthread_mutex_unlock(&mut);

    //printf("RPM\t: %.2lf (RPM)\n", *rpmVal);
  }
}
// 연료의 양을 계산하는 함수
// 소모량 = 이동거리/8.7
void *Figures_Fuel()
{
  *fuelVal = 80;
  while (1)
  {

    double result;
    //  pthread_mutex_lock(&mut);

    result = *rpmVal * Tire_Perimeter_Ratio / 1000000.0;
    // pthread_cond_wait(&cmd, &mut);

    //  pthread_mutex_unlock(&mut);
    *fuelVal = *fuelVal - (result / 1000) / (8.7);
    usleep(1000000);

    //printf("fuel\t: %.2lf (L)\n", *fuelVal);
  }
}
// 냉각수 온도
// 실제로는 센서로 측정하지만
// 연관 관계를 나타내기위헤 속도에 맞춰서 계산함
void *Figures_Coolant()
{
  while (1)
  {

    //pthread_mutex_lock(&mut);
    if (*speedVal == 0.0)
    {
      *coolantVal = 0.0;
    }
    else if (*speedVal > 0 && *speedVal <= 80)
    {
      *coolantVal = 80.0;
    }
    else if (*speedVal > 80 && *speedVal <= 140)
    {
      *coolantVal = 85.0;
    }
    else if (*speedVal > 140 && *speedVal <= 160)
    {
      *coolantVal = 90.0;
    }
    else if (*speedVal > 160 && *speedVal < 200)
    {
      *coolantVal = 95.0;
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

  re = pthread_create(&threads[0], NULL, Figures_Speed, NULL);
  if (re < 0)
  {
    //printf("threads[0] Figures_Speed error\n");
    exit(0);
  }
  re = pthread_create(&threads[1], NULL, Figures_RPM, NULL);
  if (re < 0)
  {
    //printf("threads[1] Figures_Speed error\n");
    exit(0);
  }
  re = pthread_create(&threads[2], NULL, Figures_Fuel, NULL);
  if (re < 0)
  {
    //printf("threads[2] Figures_Speed error\n");
    exit(0);
  }
  re = pthread_create(&threads[3], NULL, Figures_Coolant, NULL);
  if (re < 0)
  {
    //printf("threads[3] Figures_Speed error\n");
    exit(0);
  }

  sleep(5000);
  pthread_mutex_destroy(&mut);
  pthread_cond_destroy(&cmd);
  pthread_mutex_destroy(&rpmut);
  return 0;
}