#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio_ext.h>

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

// 온도 RPM 가스 냉각수 온도 계산 관련
#define Fan_Perimeter_Ratio 1850
#define Gas_efficiency 9
#define InitTemp 80
#define InitCoolingLevel 3
#define Min 60
#define InitGas 80

#define SIZE 256
//표시될 data들 프로세스간 통신이 될때 사용됨
int temp = 200;
int rpm = 6000;
int gas = 100;
int coolant = 1000;

// 프로세스 통신에 필요한 int 형 배열과 버퍼
int pipe1[2], pipe2[2], pipe3[2], pipe4[2], pipe5[2];
int buffer1, buffer2, buffer3, buffer4, buffer5;
int Temp = 80;
int Cooling = 3;
int Gas = InitGas;
int Coolant = 0;
int RPM = 0;

int sync_th = 0;

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

//동기화를 위한 tool
pthread_mutex_t mut;
pthread_mutex_t rpmut;
pthread_cond_t cmd;
pthread_mutex_t cursorMutex;

//시뮬레이션 관련 함수
void simulate();
void simulMenu();

//패널 업데이트 함수
void updatePanel(int temp, int rpm, int gas, int coolant);

//시뮬레이션 메뉴 값 범위 에러 함수
void range_error();

//알람 시그널 함수
void stopThread();

void *readValue(void *); //프로세스 통신용 함수
void *updateTemp(void *);
void *updateRPM(void *);
void *updateGas(void *);
void *updateCoolant(void *);

//계산 함수들
void *Figures_Temp(void *);
void *Figures_RPM(void *);
void *Figures_Gas(void *);
void *Figures_Coolant(void *);
void run_thread();

int main()
{
  pid_t pid;
  // 파이프 생성및 오류 검사
  if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1 || pipe(pipe4) == -1 || pipe(pipe5) == -1)
  {
    perror("pipe failed\n");
    exit(0);
  }
  // 자식 프로세스 생성 및 오류 검사
  if ((pid = fork()) < 0)
  {
    perror("fork error\n");
    exit(1);
  }
  else if (pid == 0)
  {
    //자식 프로세스
    run_thread();
  }
  else
  {
    // 부모 프로세스
    signal(SIGALRM, stopThread);
    pthread_mutex_init(&cursorMutex, NULL);

    initPanel();
    simulate();
  }
  return 0;
}

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
  for (i = 10; i > 0; i--)
  {
    move_cur(TEMP_X - 4, i + 5);
    if (i % 2 != 0)
      printf("%3d", (NAME_SPACE - 3 - i) * 2);
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
  {
  }
}

//시뮬레이션 메뉴 함수
void simulMenu()
{
  int input = -1;
  int p1Temp = 0;
  int p1Cool = 0;
  int check;
  int i = 0; //for 문 제어 변수

  close(pipe1[0]);
  while (1)
  {

    move_cur(0, SIMUL_SPACE - 1);
    printf("                        ");
    move_cur(0, SIMUL_SPACE);
    printf("                                                         ");
    move_cur(0, SIMUL_SPACE);
    printf("Please input temperature (10~20) >>");
    __fpurge(stdin); //리눅스에서 __fpurge(stdin)
    scanf("%d", &p1Temp);
    move_cur(0, SIMUL_SPACE);
    printf("                                                         ");
    if (p1Temp <= 20 && p1Temp >= 10)
    {
      move_cur(0, SIMUL_SPACE - 1);
      printf("                        ");
      move_cur(0, SIMUL_SPACE);
      printf("Please input Cooling level (1~3) >>");
      __fpurge(stdin); //리눅스에서 __fpurge(stdin)
      scanf("%d", &p1Cool);
      if (p1Cool >= 1 && p1Cool <= 3)
      {
        //이부분에 프로세스간 통신 작성하면 됨
        write(pipe1[1], &p1Temp, sizeof(int));
        write(pipe1[1], &p1Cool, sizeof(int));
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
  printf("Please insert correct input");
}

//value에 따라 패널을 업데이트 해주는 thread들
void *updateTemp(void *arg)
{
  int i = 0;
  int k = 0;
  int tempChart = 0;
  int nread = 0;

  //close(pipe2[1]);
  while (1)
  {
    if (sync_th != 0)
      continue;
    fcntl(pipe2[0], F_SETFL, O_NONBLOCK);

    switch (nread = read(pipe2[0], &buffer2, sizeof(int)))
    {
    case -1:
      if (errno == EAGAIN)
      {
        break;
      }
    default:
      temp = buffer2;
      break;
    }

    pthread_mutex_lock(&cursorMutex);

    tempChart = (int)((double)temp / 3);
    for (i = 0; i < 10; i++)
    {
      move_cur(TEMP_X, CHART_SPACE - i);
      printf("  ");
    }
    for (i = 0; i < tempChart; i++)
    {
      move_cur(TEMP_X, CHART_SPACE - i);
      printf("O");
    }
    move_cur(TEMP_X - 2, VALUE_SPACE);
    printf("%5d", temp);

    sync_th++;
    pthread_mutex_unlock(&cursorMutex);
  }
}
void *updateRPM(void *arg)
{
  int i = 0;
  int rpmChart = 0;
  int nread = 0;

  //close(pipe3[1]);
  while (1)
  {
    if (sync_th != 1)
      continue;
    fcntl(pipe3[0], F_SETFL, O_NONBLOCK);

    switch (nread = read(pipe3[0], &buffer3, sizeof(int)))
    {
    case -1:
      if (errno == EAGAIN)
      {
        break;
      }
    default:
      rpm = buffer3;
      break;
    }
    pthread_mutex_lock(&cursorMutex);

    rpmChart = (int)((double)rpm / 600);
    for (i = 0; i < 12; i++)
    {
      move_cur(RPM_X, CHART_SPACE - i);
      printf("  ");
    }
    for (i = 0; i < rpmChart; i++)
    {
      move_cur(RPM_X, CHART_SPACE - i);
      printf("O");
    }
    move_cur(RPM_X - 2, VALUE_SPACE);
    printf("%5d", rpm);
    sync_th++;
    pthread_mutex_unlock(&cursorMutex);
  }
}
void *updateGas(void *arg)
{
  int i = 0;
  int gasChart = 0;
  int nread = 0;

  //  close(pipe4[1]);
  while (1)
  {
    if (sync_th != 2)
      continue;
    fcntl(pipe4[0], F_SETFL, O_NONBLOCK);

    switch (nread = read(pipe4[0], &buffer4, sizeof(int)))
    {
    case -1:
      if (errno == EAGAIN)
      {
        break;
      }
    default:
      gas = buffer4;
      break;
    }

    pthread_mutex_lock(&cursorMutex);

    gasChart = (int)((double)gas / 10);
    for (i = 0; i < 10; i++)
    {
      move_cur(GAS_X, CHART_SPACE - i);
      printf("  ");
    }
    for (i = 0; i < gasChart; i++)
    {
      move_cur(GAS_X, CHART_SPACE - i);
      printf("O");
    }
    move_cur(GAS_X - 2, VALUE_SPACE);
    printf("%5d", gas);
    sync_th++;
    pthread_mutex_unlock(&cursorMutex);
  }
}
void *updateCoolant(void *arg)
{
  int i = 0;
  int coolantChart = 0;
  int nread = 0;

  //  close(pipe5[1]);
  while (1)
  {
    if (sync_th != 3)
      continue;
    fcntl(pipe5[0], F_SETFL, O_NONBLOCK);

    switch (nread = read(pipe5[0], &buffer5, sizeof(int)))
    {
    case -1:
      if (errno == EAGAIN)
      {
        break;
      }
    default:
      coolant = buffer5;
      break;
    }

    pthread_mutex_lock(&cursorMutex);

    coolantChart = (int)((double)coolant / 10);
    for (i = 0; i < 10; i++)
    {
      move_cur(COOLANT_X, CHART_SPACE - i);
      printf("  ");
    }
    for (i = 0; i < coolantChart; i++)
    {
      move_cur(COOLANT_X, CHART_SPACE - i);
      printf("O");
    }
    move_cur(COOLANT_X - 2, VALUE_SPACE);
    printf("%5d", coolant);
    sync_th = 0;
    pthread_mutex_unlock(&cursorMutex);
  }
}
void stopThread()
{

  pthread_mutex_lock(&cursorMutex);
  sleep(1);

  move_cur(0, SIMUL_SPACE + 1);
  exit(0);
}
// 온도값을 계산하는 함수
// 초기에 온도값과 Cooling Level값을 다른 프로세스에서 받아와 계산함
void *Figures_Temp(void *arg)
{
  //  close(pipe1[1]);
  //close(pipe2[0]);

  int nread = 0;
  read(pipe1[0], &buffer1, sizeof(int));
  Temp = buffer1;
  read(pipe1[0], &buffer1, sizeof(int));
  Cooling = buffer1;
  while (1)
  {
    /*fcntl(pipe1[0], F_SETFL, O_NONBLOCK);
		fcntl(pipe6[0], F_SETFL, O_NONBLOCK);
		switch (nread = read(pipe1[0], &buffer1, sizeof(int))) {
		case -1:
		if (errno == EAGAIN) {
		break;
		}
		default:
		Temp = buffer1;
		break;
		}
		switch (nread = read(pipe6[0], &buffer6, sizeof(int))) {
		case -1:
		if (errno == EAGAIN) {
		break;
		}
		default:
		Cooling = buffer6;
		break;
		}*/

    pthread_mutex_lock(&mut);
    if (Temp < 5)
    {
      Temp += Cooling;
    }
    else if (Temp >= 5)
    {
      Temp -= Cooling;
    }
    write(pipe2[1], &Temp, sizeof(int));
    //pthread_cond_signal(&cmd);
    pthread_mutex_unlock(&mut);
    //  printf("Temp\t: %.2lf (km/h)\n", Temp);
    sleep(1);
  }
}
// RPM 값을 계산하는 함수
// 스레드로 계산되는 온도값을 읽어와 순간 RPM을 계산하는 함수
// RPM = 속도 * 1000000 / 60분 / Fan의 지름
void *Figures_RPM(void *arg)
{
  //close(pipe3[0]);

  while (1)
  {

    //pthread_mutex_lock(&mut);
    pthread_mutex_lock(&rpmut);
    RPM = Temp * 10 * 1000000 / Min / Fan_Perimeter_Ratio;
    write(pipe3[1], &RPM, sizeof(int));
    //  pthread_cond_wait(&cmd, &mut);
    pthread_mutex_unlock(&rpmut);
    //  pthread_mutex_unlock(&mut);
    sleep(1);
    //  printf("RPM\t: %.2lf (RPM)\n", RPM);
  }
}
// 가스의 양을 계산하는 함수
// 소모량 = 전력소모량/8.7
void *Figures_Gas(void *arg)
{

  //close(pipe4[0]);
  while (1)
  {

    double result;
    //pthread_mutex_lock(&mut);

    result = RPM * Fan_Perimeter_Ratio / 1000000;

    //  pthread_cond_wait(&cmd, &mut);

    //  pthread_mutex_unlock(&mut);
    Gas = Gas - (result / 1000) / Gas_efficiency;
    write(pipe4[1], &Gas, sizeof(int));
    //printf("gas\t: %.2lf (L)\n", Gas);
    sleep(1);
  }
}
// 냉각수 온도
// 실제로는 센서로 측정하지만
// 연관 관계를 나타내기위헤 온도에 맞춰서 계산함
void *Figures_Coolant(void *arg)
{
  //close(pipe5[0]);

  while (1)
  {

    //pthread_mutex_lock(&mut);
    if (Temp == 0)
    {
      Coolant = 0;
    }
    else if (Temp > 0 && Temp <= 4)
    {
      Coolant = 0.0;
    }
    else if (Temp > 0 && Temp <= 8)
    {
      Coolant = 0.0;
    }
    else if (Temp > 8 && Temp <= 14)
    {
      Coolant = 5.0;
    }
    else if (Temp > 14 && Temp <= 16)
    {
      Coolant = 10.0;
    }
    else if (Temp > 16 && Temp < 20)
    {
      Coolant = 15.0;
    }
    write(pipe5[1], &Coolant, sizeof(int));
    //  pthread_cond_wait(&cmd, &mut);
    //  pthread_mutex_unlock(&mut);
    //printf("Coolant\t: %.2lf (℃  )\n", Coolant);
    sleep(1);
  }
}
void run_thread()
{
  int re = 0;
  pthread_mutex_init(&mut, NULL);
  pthread_mutex_init(&rpmut, NULL);
  pthread_cond_init(&cmd, NULL);
  pthread_t thread1, thread2, thread3, thread4;

  re = pthread_create(&thread1, NULL, Figures_Temp, NULL);
  if (re < 0)
  {
    printf("threads[0] Figures_Temp error\n");
    exit(0);
  }
  re = pthread_create(&thread2, NULL, Figures_RPM, NULL);
  if (re < 0)
  {
    printf("threads[1] Figures_RPM error\n");
    exit(0);
  }
  re = pthread_create(&thread3, NULL, Figures_Gas, NULL);
  if (re < 0)
  {
    printf("threads[2] Figures_Gas error\n");
    exit(0);
  }
  re = pthread_create(&thread4, NULL, Figures_Coolant, NULL);
  if (re < 0)
  {
    printf("threads[3] Figures_Coolant error\n");
    exit(0);
  }
  sleep(5000);
  pthread_mutex_destroy(&mut);
  pthread_cond_destroy(&cmd);
  pthread_mutex_destroy(&rpmut);
}
