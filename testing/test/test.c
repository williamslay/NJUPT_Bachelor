#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// 功能函数声明
void exchange(int *a, int *b);  // 交换a，b的值最好的方法

// 测试函数声明
void test1(void);  // 测试函数1：交换a，b的值
void test2(void);  // 测试函数2：C语言预定义宏
void test3(void);  // 测试函数3：C89与C99标准的区别
void test4(void);  // 测试函数4：输出当前编译器数据类型范围
void test5(void);

// 菜单
int menu(void);

/*=============================================================================
               主函数
==============================================================================*/
int main(void)
{
 int cmd = 0;

 for (;;)      // 死循环，等价于while(1)
 {
   cmd = menu(); // 读取用户输入的指令
   switch (cmd)
   {
     case 1 : test1();     break;  // 测试函数1：交换a，b的值
     case 2 : test2();     break;  // 测试函数2：C语言预定义宏
     case 3 : test3();     break;  // 测试函数3：C89与C99标准的区别
     case 4 : test4();     break;  // 测试函数4：输出当前编译器数据类型范围
     case 5 : test5();     break;
     case -1: exit(0);      break;  // 退出程序
     case -2: system("cls"); break;  // 清空屏幕
     default:break;
   }
 }

 return 0;
}

/*=============================================================================
               以下是功能函数实体
==============================================================================*/
/* 交换a，b的值 */
void exchange(int *a, int *b)
{
 *a = *a^*b;
 *b = *a^*b;
 *a = *a^*b;
}

/*=============================================================================
               以下是测试函数实体
==============================================================================*/
/* 交换a，b的值 */
void test1(void)
{
 system("cls");
 int a = 13, b = 14;
 printf("交换之前a = %d, b = %d\n", a, b);
 exchange(&a, &b);
 printf("交换之后a = %d, b = %d\n", a, b);
}

/* C语言预定义宏 */
void test2(void)
{
 system("cls");
 printf("当前代码行号：%d\n", __LINE__);
 printf("当前编译日期：%s\n", __DATE__);
 printf("当前编译时间：%s\n", __TIME__);
 printf("当前文件名称：%s\n", __FILE__);
 printf("当前函数名称：%s\n", __FUNCTION__);
}

/* C89与C99标准的区别 */
void test3(void)
{
 int i;
 system("cls");
 // C99允许在for循环内定义循环变量，而C89不允许
 // gcc -std=c99 test.c -o test.exe
 // for (int i = 0; i < 5; i++)  
 for (i = 0; i < 5; i++)
 {
   printf("hello world!\n");
 }
 test4();
}

/* 输出当前编译器数据类型范围 */
void test4(void)
{
 system("cls");
 printf("char的范围为: %d —— %d\n", CHAR_MIN, CHAR_MAX);
 printf("unsigned char的范围为: %u —— %u\n\n", 0, UCHAR_MAX);

 printf("short的范围为: %hd —— %hd\n", SHRT_MIN, SHRT_MAX);
 printf("unsigned short的范围为: %hu —— %hu\n\n", 0, USHRT_MAX);

 printf("int的范围为: %d —— %d\n", INT_MIN, INT_MAX);
 printf("unsigned int的范围为: %u —— %u\n\n", 0, UINT_MAX);

 printf("long的范围为: %ld —— %ld\n", LONG_MIN, LONG_MAX);
 printf("unsigned long的范围为: %lu —— %lu\n\n", 0, ULONG_MAX);

 printf("long long的范围为: %lld —— %lld\n", LLONG_MIN, LLONG_MAX);
 printf("unsigned long long的范围为: %llu —— %llu\n\n", 0, ULLONG_MAX);
}

/* 编译区别函数 */
void test5(void)
{
 int buf = 0;
 system("cls");
 fgets(buf,10,stdin);
 if (buf>0)
 {
 printf("%d",buf);
 test3(); 
 }else{
  printf("%d",buf*2);
  test4(); 
 } 
}

/*=============================================================================
               以下是菜单函数
==============================================================================*/
int menu(void)
{
 int cmd = 0;
 char buf[10] = {0};

 // 菜单信息输出
 printf("=============================菜单==============================\n");
 printf("[01]测试：交换a，b的值\n");
 printf("[02]测试：C语言预定义宏\n");
 printf("[03]测试：C89与C99标准的区别\n");
 printf("[04]测试：输出当前编译器数据类型范围\n");
 printf("[-1]退出\n");
 printf("[-2]清屏\n");
 printf("===============================================================\n");
 do{
   printf("请输入您的选择：");
   fgets(buf,10,stdin);
   cmd = atoi(buf);
 }while (cmd < 1 && cmd > 4);

 return cmd;
}
