#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include<pthread.h>
#include<cmath>
#include <iomanip>
#include<algorithm>
#define S 500     //S是点的数量
using namespace std;
//全局信息————————————————————————————————————————————————————————————————————
pthread_cond_t all_finished;
pthread_mutex_t lock1;
//pthread_rwlock_t rw_lock;
int share_data = 0;
bool continue_loop = true;
double error_array[18] = {100};
//全局信息————————————————————————————————————————————————————————————————————————


class Sector//定义扇形
{
	public:

		Sector()//构造函数
		{
      for(int i = 0; i < S; ++i)
      {
        for(int j = 0; j < S; ++j)
        {
          mat0[i][j] = 0;
          mat1[i][j] = 0;
        }
      }
      mat0[S/10-1][S/10-1] = 100;
			mat1[S/10-1][S/10-1] = 100;
      sum_of_dots = 0;
			for(int i = 0; i < S; ++i)
      {
        limit[i] = (int)sqrt(S*S-i*i);
        sum_of_dots += limit[i];
      }
		}
		void update_row(int flag, int row)//更新一行的温度，row是行数，flag是更新哪个矩阵
		{

			if(flag==0)
				for(int j = 1; j < limit[row]-1; ++j)
				{
					//if(row == S/10-1 && j == S/10-1)
					//	continue;
					mat0[row][j] = (mat1[row-1][j]+mat1[row+1][j]+mat1[row][j-1]+mat1[row][j+1])/4;
				}
			else
				for(int j = 1; j < limit[row]-1; ++j)
				{
					//if(row == S/10-1 && j == S/10-1)
					//	continue;
					mat1[row][j] = (mat0[row-1][j]+mat0[row+1][j]+mat0[row][j-1]+mat0[row][j+1])/4;
				}
        if(row == S/10 -1)
          mat0[S/10 - 1][S/10 - 1] = mat1[S/10 - 1][S/10 - 1] = 100;
		}
    void update_nrow(int flag, int start, int end)//更新n行，flag是矩阵，start是开始，end是最后一行
    {
      if(start <= 1)
        start = 1;
      if(end >= S)
        end = S-1;
      for(int i = start; i <= end; ++i)
				update_row(flag,i);
    }
    double diff(int x, int y)
    {
      return abs(mat0[x][y] - mat1[x][y]);
    }
    void count_area()
    {
      int sum = 0;
      for(int i = 0; i < S; ++i)
      {
        for(int j  = 0; j < limit[i]; ++j)
        {
          sum += (mat0[i][j]>5)?1:0;
        }
      }
      cout<<(int)((float)sum/sum_of_dots*100)<<'%'<<endl;
    }
    int limit[S];
    int sum_of_dots;
	private:
		double mat0[S][S];
		double mat1[S][S];
};
Sector s;//初始化扇形

class Args
{
public:
  int start;
  int end;
  double error;
  int node_n;
};
void *cal(void *_arg)
{
  //cout<<"进程已启动"<<endl;
  Args *arg = (Args *)_arg;
  int flag = 0;

  while(continue_loop)
  {
    flag = 1 - flag;
    s.update_nrow(flag, arg->start, arg->end);
    pthread_mutex_lock(&lock1);//加锁
    share_data += 1;
    if(share_data < arg->node_n)
    {
      pthread_cond_wait(&all_finished, &lock1);
      pthread_mutex_unlock(&lock1);//解锁
    }
    else
    {
      pthread_mutex_unlock(&lock1);//解锁
      share_data = 0;
      for(int i = 0; i < 3; ++i)
      {
        for(int j = 0; j < 3; ++j)
        {
          error_array[3*i+j] = s.diff(S/10-2+i, S/10-2+j);
        }
        for(int j = 0; j < 3; ++j)
        {
          error_array[3*i+j+9] = s.diff(S/5-2+i, S/5-2+j);
        }
        continue_loop = *max_element(error_array, error_array + 18) > arg->error;
      }
      pthread_mutex_unlock(&lock1);//解锁
      pthread_cond_broadcast(&all_finished);
    }
  }
	return NULL;
}
int main(int argc, char** argv)
{
  double error = atof(argv[1]);
  int node_n = atoi(argv[2]);
  Args *arg;
  int sum = s.sum_of_dots;
  int slide_dot[node_n+1];
  for(int i = 0; i < node_n;++i)
  {
    int target = sum/node_n*i;
    int temp_sum = 0;
    for(int j = 0; j < S; ++j)
    {
      if(temp_sum >= target)
      {
        slide_dot[i] = j;
        break;
      }
      else
        temp_sum += s.limit[j];
    }
  }
  //计算参数————————————————————————————————————————————————————————————————————

  //初始化锁，条件变量，读写锁——————————————————————————————————————————————
  pthread_cond_init(&all_finished, NULL);
  pthread_mutex_init(&lock1, NULL);
  //初始化锁，条件变量，读写锁——————————————————————————————————————

  //创建进程————————————————————————————————————————————————————————————————————
  pthread_t *pid;
	pid = (pthread_t *)malloc(node_n * sizeof(pthread_t));
  for (int i = 0; i < node_n; ++i) {
    arg = (Args *)malloc(sizeof(Args));
		arg->error = error;
    arg->start = slide_dot[i];
    arg->end = i!=node_n-1?slide_dot[i+1]-1:S;
    arg->node_n = node_n;
		pthread_create(&pid[i], NULL, cal, (void *)arg);
	}
  //创建进程——————————————————————————————————————————————————————————————————————

	for (int i = 0; i < node_n; ++i) {
		pthread_join(pid[i], NULL);
	}
  s.count_area();
  //cout<<"已join进程"<<endl;
  pthread_mutex_destroy(&lock1);
  pthread_cond_destroy(&all_finished);
	free(pid);

  //pthread_rwlock_destroy(&rw_lock);
	return 0;
}
