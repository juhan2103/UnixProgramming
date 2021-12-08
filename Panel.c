#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

//인터페이스 좌표 관련
#define MAX_X 75
#define TEMP_X 15
#define RPM_X 30
#define GAS_X 45
#define COOLANT_X 60
#define CHART_SPACE 15
#define NAME_SPACE 16
#define VALUE_SPACE 18
#define SIMUL_SPACE 22

//시간 관련
#define UPDATE_TURM 1
#define SIMUL_TIME 15

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

//통신 관련 함수
void init_shm();

//커서 함수
void move_cur(int, int);

//초기화 관련 함수
void drawBoard();
void drawInfo();
void initPanel();

//쓰레드 함수
pthread_t readValueth;
pthread_t updateTempth;
pthread_t updateRPMth;
pthread_t updateGasth;
pthread_t updateCoolantth;
void *readValue(void *); //프로세스 통신용 함수
void *updateTemp(void *);
void *updateRPM(void *);
void *updateGas(void *);
void *updateCoolant(void *);

pthread_mutex_t cursorMutex;

int sync_th = 0;

//시뮬레이션 관련 함수
void simulate();
void simulMenu();

//패널 업데이트 함수
void updatePanel(int temp, int rpm, int gas, int coolant);

//시뮬레이션 메뉴 값 범위 에러 함수
void range_error();

//알람 시그널 함수
void stopThread()
{

  pthread_mutex_lock(&cursorMutex);
  sleep(1);

  move_cur(0, SIMUL_SPACE + 1);
  exit(0);
}

int main(void)
{
  int pid;
  static struct sigaction act;

  //초기화 부분
  signal(SIGALRM, stopThread);
  init_shm();
  pthread_mutex_init(&cursorMutex, NULL);
  initPanel();

  pid = fork();
  if (pid < 0)
    exit(0);
  if (pid == 0)
  {
    //자식 프로세스
    execl("./rsm", (char *)0);
  }
  //부모 프로세스
  else
    simulate();

  pthread_mutex_destroy(&cursorMutex);

  return 0;
}

//Shared Memory를 사용하기 위한 초기화
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

//콘솔 환경에서 인터페이스를 그리기위해
//커서를 쉽게 옮길 수 있도록 구현한 함수
void move_cur(int x, int y)
{
  printf("\033[%d;%df", y, x);
}

//초기 화면을 지우고 인터페이스의 틀을 그려주는 함수
void drawBoard()
{
  int i = 1;

  system("clear"); //윈도우는 cls, 리눅스는 clear
  move_cur(0, 0);
  printf("===========================================================================");
  for (i = 1; i < 20; i++)
  {
    move_cur(0, i);
    printf("=");
    move_cur(MAX_X, i);
    printf("=");
  }
  move_cur(0, 20);
  printf("===========================================================================");
}

//Panel 에서 보여줘야할 그래프를 초기화 해주는 함수
void drawInfo()
{
  int x = 0;
  int y = 0;
  int i = 0;

  //온도 부분 초기화
  for (i = 12; i > 0; i--)
  {
    move_cur(TEMP_X - 4, i + 3);
    if (i % 2 != 0)
      printf("%2d", (NAME_SPACE - 3 - i) * 2);
  }
  move_cur(TEMP_X - 2, NAME_SPACE);
  printf("%5s", "TEMP");
  move_cur(TEMP_X - 2, VALUE_SPACE);
  printf("%3d", 0);

  //RPM부분 초기화
  for (i = 12; i > 0; i--)
  {
    move_cur(RPM_X - 4, i + 3);
    if (i % 2)
      printf("%3d", (NAME_SPACE - 3 - i) / 2);
  }
  move_cur(RPM_X - 2, NAME_SPACE);
  printf("%5s", "RPM");
  move_cur(RPM_X - 2, VALUE_SPACE);
  printf("%3d", 0);

  //가스 부분 초기화
  move_cur(GAS_X - 4, NAME_SPACE - 10);
  printf("%3c", 'F');
  move_cur(GAS_X - 4, NAME_SPACE - 1);
  printf("%3c", 'E');
  move_cur(GAS_X - 2, NAME_SPACE);
  printf("%5s", "GAS");
  move_cur(GAS_X - 2, VALUE_SPACE);
  printf("%3d", 0);

  //냉각수 부분 초기화
  move_cur(COOLANT_X - 2, NAME_SPACE);
  printf("%5s", "COOLANT");
  move_cur(COOLANT_X - 2, VALUE_SPACE);
  printf("%3d", 0);
}

//Panel 부분을 초기화 해주는 함수
void initPanel()
{
  drawBoard();
  drawInfo();
}

//시뮬레이팅을 위한 함수
void simulate()
{
  int input = -1;

  move_cur(0, SIMUL_SPACE);
  printf("What do you want? (0 : exit, 1 : simulation) >>");
  __fpurge(stdin); //리눅스에서 __fpurge(stdin)
  scanf("%d", &input);
  switch (input)
  {
  case 0:
    move_cur(0, 0);
    system("clear");
    exit(0);
  case 1:
    simulMenu();
    break;
  default:
    range_error();
  }
  while (1)
    ;
}

//시뮬레이션 메뉴 함수
void simulMenu()
{
  int ac = -1;
  int sp = -1;
  while (1)
  {
    move_cur(0, SIMUL_SPACE - 1);
    printf("                        ");
    move_cur(0, SIMUL_SPACE);
    printf("                                                         ");
    move_cur(0, SIMUL_SPACE);
    printf("Please input temperature (10~20) >>");
    __fpurge(stdin); //리눅스에서 __fpurge(stdin)
    scanf("%d", &sp);
    move_cur(0, SIMUL_SPACE);
    printf("                                                         ");
    if (sp <= 20 && sp >= 10)
    {
      move_cur(0, SIMUL_SPACE - 1);
      printf("                        ");
      move_cur(0, SIMUL_SPACE);
      printf("Please input Cooling level (1~3) >>");
      __fpurge(stdin); //리눅스에서 __fpurge(stdin)
      scanf("%d", &ac);
      if (ac >= 1 && ac <= 3)
      {
        *tempVal = sp;
        *coolVal = ac;
        *gasVal = 80;
        //스레드 생성 및 알람 등록

        if (pthread_create(&updateTempth, NULL, &updateTemp, NULL) < 0)
          exit(2);
        if (pthread_create(&updateRPMth, NULL, &updateRPM, NULL) < 0)
          exit(2);
        if (pthread_create(&updateGasth, NULL, &updateGas, NULL) < 0)
          exit(2);
        if (pthread_create(&updateCoolantth, NULL, &updateCoolant, NULL) < 0)
          exit(2);

        alarm(SIMUL_TIME);
      }
      else
        range_error();

      break;
    }
    else
      range_error();
    break;
  }
}

//시뮬레이션의 값 범위 에러 메시지 함수
void range_error()
{
  move_cur(0, SIMUL_SPACE - 1);
  printf("Please insert correct input");
}

//value에 따라 패널을 업데이트 해주는 thread들
void *updateTemp(void *arg)
{
  int i = 0;
  int tempChart = 0;
  while (1)
  {
    if (sync_th != 0)
      continue;
    pthread_mutex_lock(&cursorMutex);
    tempChart = (int)((double)*tempVal / 2);
    for (i = 0; i < 10; i++)
    {
      move_cur(TEMP_X, CHART_SPACE - i);
      printf("  ");
    }
    for (i = 0; i < tempChart; i++)
    {
      move_cur(TEMP_X, CHART_SPACE - i);
      printf("#");
    }
    move_cur(TEMP_X - 2, VALUE_SPACE);
    printf("%5d", *tempVal);
    sync_th++;
    pthread_mutex_unlock(&cursorMutex);
  }
}
void *updateRPM(void *arg)
{
  int i = 0;
  int rpmChart = 0;
  while (1)
  {
    if (sync_th != 1)
      continue;
    pthread_mutex_lock(&cursorMutex);
    rpmChart = (int)((double)*rpmVal / 600);
    for (i = 0; i < 12; i++)
    {
      move_cur(RPM_X, CHART_SPACE - i);
      printf("  ");
    }
    for (i = 0; i < rpmChart; i++)
    {
      move_cur(RPM_X, CHART_SPACE - i);
      printf("#");
    }
    move_cur(RPM_X - 2, VALUE_SPACE);
    printf("%5d", *rpmVal);
    sync_th++;
    pthread_mutex_unlock(&cursorMutex);
  }
}
void *updateGas(void *arg)
{
  int i = 0;
  int gasChart = 0;
  while (1)
  {
    if (sync_th != 2)
      continue;
    pthread_mutex_lock(&cursorMutex);
    gasChart = (int)((double)*gasVal / 10);
    for (i = 0; i < 10; i++)
    {
      move_cur(GAS_X, CHART_SPACE - i);
      printf("  ");
    }
    for (i = 0; i < gasChart; i++)
    {
      move_cur(GAS_X, CHART_SPACE - i);
      printf("#");
    }
    move_cur(GAS_X - 2, VALUE_SPACE);
    printf("%5d", *gasVal);
    sync_th++;
    pthread_mutex_unlock(&cursorMutex);
  }
}
void *updateCoolant(void *arg)
{
  int i = 0;

  int coolantChart = 0;
  while (1)
  {
    if (sync_th != 3)
      continue;
    pthread_mutex_lock(&cursorMutex);
    coolantChart = (int)((double)*coolantVal / 2);
    for (i = 0; i < 10; i++)
    {
      move_cur(COOLANT_X, CHART_SPACE - i);
      printf("  ");
    }
    for (i = 0; i < coolantChart; i++)
    {
      move_cur(COOLANT_X, CHART_SPACE - i);
      printf("#");
    }
    move_cur(COOLANT_X - 2, VALUE_SPACE);
    printf("%5d", *coolantVal);
    sync_th = 0;
    pthread_mutex_unlock(&cursorMutex);
  }
}