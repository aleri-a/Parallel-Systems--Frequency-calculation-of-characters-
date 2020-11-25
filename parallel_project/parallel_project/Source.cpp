
#include<mpi.h>
#include<stdio.h>
#include <string.h>
#include <string>
#include <omp.h>
//#include <Windows.h>
#include <time.h>

#include<ctime>

using namespace std;
#define TAG 45
#define NUM_SEND 10000
#define azbuka 15
#define NUMCHAR 7002000

string randomHex(int num);
void Sequential(string s);

static double get_time(MPI_Comm comm) {
	MPI_Barrier(comm);
	return MPI_Wtime();
	MPI_Barrier(comm);
}



static int  recv_available() {
	MPI_Status status;
	MPI_Recv(NULL, 0, MPI_INT, MPI_ANY_SOURCE, TAG + 1, MPI_COMM_WORLD, &status);
	return status.MPI_SOURCE;
}
static void send_available(int dst) {
	MPI_Send(NULL, 0, MPI_INT, dst, TAG + 1, MPI_COMM_WORLD);
}

void Emiter(string txt) //emiter=0, colector=1; other are workers
{
	
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	unsigned brojac = 0;

	
	int tmp = txt.length() % NUM_SEND;
	if (tmp != 0)
	{
		int brojac = NUM_SEND - tmp;
		txt.append(brojac, '0');
	}

	int size, dst;
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	while (brojac < txt.length())
	{
		dst = recv_available();
		MPI_Send(&txt[brojac], NUM_SEND, MPI_CHAR, dst, TAG, MPI_COMM_WORLD);
		brojac += NUM_SEND;
	}

	char txtEnd[NUM_SEND] = { '0' };
	MPI_Status st;

	for (int jj = 2; jj < size; jj++)
	{
		MPI_Send(&txtEnd, NUM_SEND, MPI_CHAR, jj, TAG, MPI_COMM_WORLD);
		int end;
		MPI_Recv(&end, 1, MPI_INT, jj, 0, MPI_COMM_WORLD, &st);
	}

	int txtEndColl[azbuka] = { -1 };
	MPI_Send(&txtEndColl, 15, MPI_INT, 1, 19, MPI_COMM_WORLD);

	return;
}
void Worker()
{
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	char s[NUM_SEND];

	while (true)
	{
		send_available(0);
		int res[15] = { 0 };

		MPI_Recv(s, NUM_SEND, MPI_CHAR, 0, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		if (s[0] == (char)'0')
		{
			int end = 99;
			MPI_Send(&end, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
			return;
		}

#pragma omp parallel for num_threads(1)

		for (int i = 0; i < NUM_SEND; i++)
		{
			int pos;
			switch (s[i]) {
			case '0': pos = 99;
				break;
			case 'A': pos = 10;
				break;
			case'B': pos = 11;
				break;
			case'C': pos = 12;
				break;
			case'D': pos = 13;
				break;
			case'E': pos = 14;
				break;
			case'F': pos = 15;
				break;
			default:
				pos = s[i] - '0';
			}
			if (pos != 99)
#pragma omp atomic
				++res[pos - 1];
		}

		int test = MPI_Send(&res, azbuka, MPI_INT, 1, 19, MPI_COMM_WORLD);
		if (test != MPI_SUCCESS)
			printf("MPI send error: %d", test);
		fflush(stdout);
	}

}

int* Collector()
{
	int* resAll = new int[15];
	for (int k = 0; k < 15; k++)
		resAll[k] = 0;

	int resThr[15];
	MPI_Status st;
	int tmp = 0;

	while (true) {
		MPI_Recv(resThr, 15, MPI_INT, MPI_ANY_SOURCE, 19, MPI_COMM_WORLD, &st);

		if (resThr[0] == -1 || st.MPI_SOURCE == 0)
		{
			return resAll;
		}
		for (int ij = 0; ij < azbuka; ij++)
			resAll[ij] += resThr[ij];
		tmp++;
	}

}

int main(int argc, char** argv)
{
	int required = MPI_THREAD_SERIALIZED, provided;
	srand(time(NULL));
	MPI_Init_thread(&argc, &argv, required, &provided);
	if (provided != required) printf("required!=provided\n");

	int rank, size;

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	string txt;

	if (rank == 0)
	{
		txt = randomHex(NUMCHAR);
		printf("\nString length: %d, num of process: %d, numb of char per worker: %d \n", NUMCHAR, size, NUM_SEND);
	}

	double start = get_time(MPI_COMM_WORLD);
	if (rank == 0) {
		Emiter(txt);
		fflush(stdout);
	}
	else if (rank == 1)
	{
		Collector();
	}
	else  Worker();
	double end = get_time(MPI_COMM_WORLD);
	fflush(stdout);
	if (rank == 0)
	{
		printf("Par elapsed time = %.10f seconds\n", end - start);
		double startSeq = omp_get_wtime();
		Sequential(txt);
		double endSeq = omp_get_wtime();
		printf("Seq elapsed time = %.10f seconds\nSpeedup = %fx\n", endSeq - startSeq, (endSeq - startSeq) / (end - start)), fflush(stdout);
	}
	MPI_Finalize();
	return 0;
}


string randomHex(int num) {

	string res = "";
	char arr[15] = { '1','2','3','4','5','6' ,'7','8','9','A', 'B', 'C', 'D', 'E', 'F' };
	for (int i = 0; i < num; i++)
	{
		int pos = rand() % 15;
		res.append(1, arr[pos]);
	}

	return res;

}


void Sequential(string s) {
	int pos;
	int res[15] = { 0 };
	for (unsigned i = 0; i < s.length(); i++)
	{
		switch (s[i]) {
		case '0': pos = 99;
			break;
		case 'A': pos = 10;
			break;
		case'B': pos = 11;
			break;
		case'C': pos = 12;
			break;
		case'D': pos = 13;
			break;
		case'E': pos = 14;
			break;
		case'F': pos = 15;
			break;
		default:
			pos = s[i] - '0';
		}
		if (pos != 99)
			res[pos - 1]++;
	}

	return;
}
