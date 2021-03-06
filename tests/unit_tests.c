#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"
#include "../dsp.h"
#include "../finger.h"

void print_pixel_array(unsigned char *data, int width, int height){
	printf("----ARRAY----\n");
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			printf(" %d", data[i*width + j]);
		}
		printf("\n");
	}
}

void print_centroid_array(struct Centroid* cents, int len){
	printf("----ARRAY----\n");
	for (int i = 0; i < len; ++i)
	{
		printf(" (%d, %d) s:%d\n", cents[i].x, cents[i].y, cents[i].size);
	}
}

int pixels_equal(unsigned char *data1, unsigned char *data2, int width, int height){
	for (int i = 0; i < width*height; ++i)
	{
		if (data1[i] != data2[i])
		{
			printf("ARRAYS NOT EQUAL AT (%d, %d)\n", i%width, i/width);
			print_pixel_array(data1, width, height);
			print_pixel_array(data2, width, height);
			return 0;
		}
	}
	return 1;
}

int centroids_equal(struct Centroid* cents1, struct Centroid* cents2, int len){
	for (int i = 0; i < len; ++i)
	{
		if (cents1[i].x != cents2[i].x ||
			cents1[i].y != cents2[i].y ||
			cents1[i].size != cents2[i].size)
		{
			printf("CENTROIDS NOT EQUAL AT %d\n", i);
			print_centroid_array(cents1, len);
			print_centroid_array(cents2, len);
			return 0;
		}
	}
	return 1;
}

/*
	Tests to make sure centroid function groups pixels
	correctly by assigning them incrementing integers
*/
void test_centroid1(void)
{
   unsigned char pixels[5*5] = {
   		0, 0, 0, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 0, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 0, 0, 0,
   };

   unsigned char ans[5*5] = {
   		0, 0, 0, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 0, 0, 0,
   		0, 2, 2, 2, 0,
   		0, 0, 0, 0, 0,
   };

   get_centroids(pixels, 5, 5);
   CU_ASSERT(pixels_equal(pixels, ans, 5, 5));
}
void test_centroid2(void)
{
   unsigned char pixels[5*5] = {
   		0, 0, 0, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 1, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 0, 0, 0,
   };

   unsigned char ans[5*5] = {
   		0, 0, 0, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 1, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 0, 0, 0,
   };

   get_centroids(pixels, 5, 5);
   CU_ASSERT(pixels_equal(pixels, ans, 5, 5));
}
void test_centroid3(void)
{
   unsigned char pixels[5*5] = {
   		0, 0, 0, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 0, 1, 0,
   		0, 1, 1, 1, 0,
   		0, 1, 0, 0, 1,
   };

   unsigned char ans[5*5] = {
   		0, 0, 0, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 0, 1, 0,
   		0, 1, 1, 1, 0,
   		0, 1, 0, 0, 3,
   };

   get_centroids(pixels, 5, 5);
   CU_ASSERT(pixels_equal(pixels, ans, 5, 5));
}

/*
	Tests to make sure centroids are correctly calculated
*/
void test_centroid4(void)
{
   unsigned char pixels[5*5] = {
   		0, 0, 0, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 1, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 0, 0, 0,
   };

   struct Centroid ans[1];
   ans[0] = (struct Centroid){2.0f, 2.0f, 7};

   struct Centroid* cents = get_centroids(pixels, 5, 5);
   CU_ASSERT(centroids_equal(cents, ans, 1));
}
void test_centroid5(void)
{
   unsigned char pixels[5*5] = {
   		0, 0, 0, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 0, 0, 0,
   		0, 1, 1, 1, 0,
   		0, 0, 0, 0, 0,
   };

   struct Centroid ans[2];
   ans[0] = (struct Centroid){2.0f, 1.0f, 3};
   ans[1] = (struct Centroid){2.0f, 3.0f, 3};

   struct Centroid* cents = get_centroids(pixels, 5, 5);
   CU_ASSERT(centroids_equal(cents, ans, 2));
}
void test_centroid6(void)
{
   unsigned char pixels[10*10] = {
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
   		0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
   		0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
   		0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
   		0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
   		0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
   		0, 0, 0, 0, 0, 0, 0, 1, 1, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
   };

   struct Centroid ans[1];
   ans[0] = (struct Centroid){4.5f, 5.0f, 16};

   struct Centroid* cents = get_centroids(pixels, 10, 10);
   CU_ASSERT(centroids_equal(cents, ans, 1));
}
void test_centroid7(void)
{
   unsigned char pixels[20*20] = {
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0,
   		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   };

   struct Centroid ans[2];
   ans[0] = (struct Centroid){3.0f, 3.0f, 16};
   ans[1] = (struct Centroid){16.0f, 16.0f, 19};

   struct Centroid* cents = get_centroids(pixels, 20, 20);
   CU_ASSERT(centroids_equal(cents, ans, 2));
}
void test_centroid8(void)
{
   unsigned char pixels[6*6] = {
         0, 0, 0, 0, 0, 0,
         0, 1, 1, 0, 1, 1,
         0, 1, 0, 0, 0, 1,
         0, 1, 1, 0, 0, 1,
         0, 0, 0, 0, 0, 1,
         0, 1, 0, 0, 1, 1,
   };

   struct Centroid ans[3];
   ans[0] = (struct Centroid){1.4f, 2.0f, 5};
   ans[1] = (struct Centroid){33.0f / 7.0f, 3.0f, 7};
   ans[2] = (struct Centroid){1.0f, 5.0f, 1};

   struct Centroid* cents = get_centroids(pixels, 6, 6);
   CU_ASSERT(centroids_equal(cents, ans, 3));
}

/*
*  Test for line intersection function
*/
void test_line1(void){
   struct Centroid points[4];
   points[0] = (struct Centroid){-1.0, 1.0, 0.0};
   points[1] = (struct Centroid){3.0, 0.0, 0.0};
   points[2] = (struct Centroid){-1.0, -1.0, 0.0};
   points[3] = (struct Centroid){2.0, 2.0, 0.0};
   struct Centroid ans = (struct Centroid){0.6, 0.6, 0.0};
   struct Centroid act = line_intersect(points[0], points[1], points[2], points[3]);
   CU_ASSERT(act.x == ans.x);
   CU_ASSERT(act.y == ans.y);
}
void test_line2(void){
   struct Centroid points[4];
   points[0] = (struct Centroid){-0.1, 10.0, 0.0};
   points[1] = (struct Centroid){0.1, -10.0, 0.0};
   points[2] = (struct Centroid){-20.0, 0.0, 0.0};
   points[3] = (struct Centroid){30.0, 0.0, 0.0};
   struct Centroid ans = (struct Centroid){0.0, 0.0, 0.0};
   struct Centroid act = line_intersect(points[0], points[1], points[2], points[3]);
   CU_ASSERT(act.x == ans.x);
   CU_ASSERT(act.y == ans.y);
}

/*
*  Tests for circle calculation
*/
void test_circle1(void){
   struct Centroid points[3];
   points[0] = (struct Centroid){-3.0, 0.0, 0.0};
   points[1] = (struct Centroid){-2.0, 1.0, 0.0};
   points[2] = (struct Centroid){-1.0, 0.0, 0.0};
   struct Circle ans = (struct Circle){-2.0, 0.0, 1.0};
   struct Circle c = get_circle(points[0], points[1], points[2]);
   CU_ASSERT(c.x == ans.x);
   CU_ASSERT(c.y == ans.y);
   CU_ASSERT(c.r == ans.r);
}

/*
*  Test for finger tracking methods
*/
void test_finger1(void){
   struct Centroid points1[5]={
      {0, 0, 0},
      {200, 100, 50},
      {100, 300, 50},
      {0, 0, 0},
      {400, 0, 50},
   };
   ft_input_centroids(points1, 5);
   struct Centroid points2[5]={
      {210, 90, 55},
      {0, 0, 0},
      {0, 0, 0},
      {370, 0, 75},
      {100, 320, 65},
   };
   ft_input_centroids(points2, 5);
   CU_ASSERT(ft_get_finger(0).size == 55);
   CU_ASSERT(ft_get_finger(1).size == 65);
   CU_ASSERT(ft_get_finger(2).size == 75);
}

/* The main() function for setting up and running the tests.
 * Returns a CUE_SUCCESS on successful running, another
 * CUnit error code on failure.
 */
int main()
{
   CU_pSuite pSuite = NULL;

   /* initialize the CUnit test registry */
   if (CUE_SUCCESS != CU_initialize_registry())
      return CU_get_error();

   /* add a suite to the registry */
   pSuite = CU_add_suite("Suite_1", NULL, NULL);
   if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* add the tests to the suite */
   /* NOTE - ORDER IS IMPORTANT - MUST TEST fread() AFTER fprintf() */
   CU_add_test(pSuite, "test pixel grouping 1", test_centroid1);
   CU_add_test(pSuite, "test pixel grouping 2", test_centroid2);
   CU_add_test(pSuite, "test pixel grouping 3", test_centroid3);
   CU_add_test(pSuite, "test centroid calculation 1", test_centroid4);
   CU_add_test(pSuite, "test centroid calculation 2", test_centroid5);
   CU_add_test(pSuite, "test centroid calculation 3", test_centroid6);
   CU_add_test(pSuite, "test centroid calculation 4", test_centroid7);
   CU_add_test(pSuite, "test centroid placement in array 1", test_centroid8);
   //CU_add_test(pSuite, "test line intersection 1", test_line1);
   //CU_add_test(pSuite, "test line intersection 2", test_line2);
   //CU_add_test(pSuite, "test circle calculation 1", test_circle1);
   CU_add_test(pSuite, "test finger tracking 1", test_finger1);

   /* Run all tests using the CUnit Basic interface */
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_cleanup_registry();
   return CU_get_error();
}