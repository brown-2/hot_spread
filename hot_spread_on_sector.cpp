#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include<pthread.h>
#include<cmath>
#include <iomanip>
#define S 500     //S是点的数量
using namespace std;
//全局信息————————————————————————————————————————————————————————————————————
pthread_cond_t all_finished;
pthread_mutex_t lock1;
//pthread_rwlock_t rw_lock;
int share_data = 0;
//全局信息————————————————————————————————————————————————————————————————————————


class Sector//定义扇形
{
	public:

		Sector()//构造函数
		{
			//给两个矩阵的温度100的点都初始化

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

			//确定扇形的边界，用一个limit数组保存边界长度
			for(int i = 0; i < S; ++i)
				limit[i] = (int)sqrt(S*S-i*i);
      sum_of_dots = 0;
      for(int i = 0; i < S;++i)
        sum_of_dots += limit[i];
		}
    int get_sum_of_dots()
    {
      return sum_of_dots;
    }

		void update_row(int flag, int row)//更新一行的温度，row是行数，flag是更新哪个矩阵
		{
			if(flag==0)
				for(int j = 1; j < limit[row]-1; ++j)
				{
					if(row == S/10-1 && j == S/10-1)
						continue;
					mat0[row][j] = (mat1[row-1][j]+mat1[row+1][j]+mat1[row][j-1]+mat1[row][j+1])/4;
				}
			else
				for(int j = 1; j < limit[row]-1; ++j)
				{
					if(row == S/10-1 && j == S/10-1)
						continue;
					mat1[row][j] = (mat0[row-1][j]+mat0[row+1][j]+mat0[row][j-1]+mat0[row][j+1])/4;
				}
		}

    void update_nrow(int flag, int start, int end)//更新n行，flag是矩阵，start是开始，end是最后一行
    {
      if(start <= 1)
        start = 1;
      if(end >= S)
        end = S-1;
      for(int i = start; i < end; ++i)
				update_row(flag,i);
    }
    void update_nrow_ntime(int start, int end, int n)//更新n行n次,参数依次为：开始行，结束行，次数
    {
      int flag = 0, count = 0;
			while(count<n)
			{
				update_nrow(flag, start, end);
				flag = 1-flag;
				++count;
			}
    }

		void update_all(int flag)//更新全部行，需指定更新哪个矩阵
		{
			for(int i = 1; i < S-1; ++i)
				update_row(flag,i);
		}
		//打印矩阵，需指定打印哪个
		void print(int flag)
		{
			if(flag==0)
				for(int i = 0; i < S; ++i)
				{
					for(int j  = 0; j < limit[i]; ++j)
					{
						cout.setf(ios::right|ios::fixed);
						cout<<setw(5)<<setprecision(2)<<mat0[i][j]<<' ';
					}
					cout<<endl;
				}
			else
				for(int i = 0; i < S; ++i)
				{
					for(int j  = 0; j < limit[i]; ++j)
					{
						cout.setf(ios::right|ios::fixed);
						cout<<setw(5)<<setprecision(2)<<mat1[i][j]<<' ';
					}
					cout<<endl;
				}
		}
		//更新n次，不需指定flag
		void update_ntimes(int n)
		{
			int flag = 0, count = 0;
			while(count<n)
			{
				update_all(flag);
				flag = 1-flag;
				++count;
			}
		}

    int limit[S];
	private:
		//mat0和mat1为辗转更新的两个矩阵
		double mat0[S][S];
		double mat1[S][S];
		//limit保存的是边界长度，limit[4]为第5行的长度（点数）。

    int sum_of_dots;
};
Sector s;//初始化扇形

class Args
{
public:
  int start;
  int end;
  int iter_n;
  int node_n;
};
void *cal(void *_arg)
{
  //cout<<"进程已启动"<<endl;
  Args *arg = (Args *)_arg;
  for(int i = 0; i < arg->iter_n; ++i)
  {
    s.update_nrow(i%2, arg->start, arg->end);
    //cout<<"第 "<<i<<"次 "<<arg->start<<"-"<<arg->end<<"行的更新已完成"<<endl;

    pthread_mutex_lock(&lock1);//加锁
    share_data += 1;
    if(share_data < arg->node_n)
    {
      pthread_cond_wait(&all_finished, &lock1);
    }
    else
    {
      //pthread_rwlock_wrlock(&rw_lock);
      share_data = 0;
      //pthread_rwlock_unlock(&rw_lock);
      pthread_cond_broadcast(&all_finished);
    }
    pthread_mutex_unlock(&lock1);//解锁
  }
	return NULL;
}
int main(int argc, char** argv)
{
  //初始化信息，得到迭代次数iter_n和线程数node_n——————————————————
  int iter_n = atoi(argv[1]);
  int node_n = atoi(argv[2]);
  //int iter_n,node_n = 3;//迭代次数和线程数
  //cout<<"已获得数据，正在返回"<<endl;
  Args *arg;
  //初始化信息，得到迭代次数iter_n和线程数node_n————————————————————

  //计算参数——————————————————————————————————————————————————————————————
  int sum = s.get_sum_of_dots();
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
  //cout<<"计算完参数"<<endl;
  //计算参数————————————————————————————————————————————————————————————————————

  //初始化锁，条件变量，读写锁——————————————————————————————————————————————
    pthread_cond_init(&all_finished, NULL);
    pthread_mutex_init(&lock1, NULL);
    //pthread_rwlock_init(&rw_lock, NULL);
    //cout<<"已初始化锁"<<endl;
  //初始化锁，条件变量，读写锁——————————————————————————————————————

  //创建进程————————————————————————————————————————————————————————————————————
  pthread_t *pid;
	pid = (pthread_t *)malloc(node_n * sizeof(pthread_t));
  for (int i = 0; i < node_n; ++i) {
    arg = (Args *)malloc(sizeof(Args));
		arg->iter_n = iter_n;
    arg->start = slide_dot[i];
    arg->end = i!=node_n-1?slide_dot[i+1]-1:S;
    arg->node_n = node_n;
		pthread_create(&pid[i], NULL, cal, (void *)arg);

	}
  //cout<<"已创建进程"<<endl;
  //创建进程——————————————————————————————————————————————————————————————————————
	for (int i = 0; i < node_n; ++i) {
		pthread_join(pid[i], NULL);
	}
  //cout<<"已join进程"<<endl;
  pthread_mutex_destroy(&lock1);
  pthread_cond_destroy(&all_finished);
	free(pid);
  //pthread_rwlock_destroy(&rw_lock);
	return 0;
}
