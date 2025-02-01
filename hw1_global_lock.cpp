/** HW 1 CSE375 Spring 2025 w/ Professor Palmieri
	Ryan Cleary **/
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <random>
#include <chrono>
using namespace std;


// Step 1 & 2
		// Define a map of types <int,float>
		// this map represents a collection of bank accounts:
		// each account has a unique ID of type int;
		// each account has an amount of fund of type float.
		// Populate the entire map with the 'insert' function
		// Initialize the map in a way the sum of the amounts of
		// all the accounts in the map is 100,000.
//One lock for the whole Map
struct Bank {
	map<int, float> accounts;
	std::mutex map_lock;
};

void create_map(Bank &bank) {
	//Create 100 accounts with 1000 balance each and do the initial sanity check
	int num_accounts = 100;
	float balance = 100000 / num_accounts;
	for (int i = 0; i < num_accounts; i++) {
		bank.accounts.insert(make_pair(i, balance));
	}
	cout << "Map created and balance is: ";
	float total = 0;
	for (int i = 0; i < 100; i++) {
		total += bank.accounts[i];
	}
	cout << total << endl;
}

// Step 3
		// Define a function "deposit" that selects two random bank accounts
		// and an amount. This amount is subtracted from the amount
		// of the first account and summed to the amount of the second
		// account. In practice, give two accounts B1 and B2, and a value V,
		// the function performs B1-=V and B2+=V.
		// The execution of the whole function should happen atomically:
		// no operation should happen on B1 and B2 (or on the whole map?)
		// while the function executes.
void deposit(Bank &bank) {
	//Locks the entire map and releases when function ends 
	std::lock_guard<std::mutex> guard(bank.map_lock);
	
	thread_local std::mt19937 generator(std::random_device{}());
	std::uniform_int_distribution<int> account_dist(0, 99);
	std::uniform_real_distribution<float> amount_dist(0.0, 100.0);

	int account1 = account_dist(generator);
	int account2 = account_dist(generator);
	float amount = amount_dist(generator);

	bank.accounts[account1] -= amount;
	bank.accounts[account2] += amount;
}


// Step 4
		// Define a function "balance" that sums the amount of all the
		// bank accounts in the map. In order to have a consistent result,
		// the execution of this function should happen atomically:
		// no other deposit operations should interleave.
float balance(Bank &bank) {
	//Locks the entire map and releases when function ends
	std::lock_guard<std::mutex> guard(bank.map_lock);
	float total = 0;
	for (int i = 0; i < 100; i++) {
		total += bank.accounts[i];
	}
	return total; 
}

// Step 5
		// Define a function 'do_work', which has a for-loop that
		// iterates for some number of iterations. In each iteration,
		// the function 'deposit' should be called with 95% of the probability;
		// otherwise (the rest 5%) the function 'balance' should be called.
		// The function 'do_work' should measure 'exec_time_i', which is the
		// time needed to perform the entire for-loop. This time will be shared with
		// the main thread once the thread executing the 'do_work' joins its execution
		// with the main thread.
double do_work(Bank &bank, int iterations) {
	//Thread local random number generator 
	thread_local std::mt19937 generator(std::random_device{}());
	std::uniform_int_distribution<int> account_dist(0, 99);
	std::uniform_int_distribution<int> probability_dist(0, 99);

	//get start and end time after work is preformed
	auto start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < iterations; i++) {
		if (probability_dist(generator) < 95) {
			deposit(bank);
		} else {
			balance(bank);
		}
	}
	auto end = std::chrono::high_resolution_clock::now();
	//return how long it took to do the work
	std::chrono::duration<double> exec_time = end - start;
	return exec_time.count();
}

// Step 6
		// The evaluation should be performed in the following way:
		// - the main thread creates #threads threads use std:threds
		// - each thread executes the function 'do_work' until completion
		// - the (main) spawning threa\d waits for all the threads to be executed
		//   << use std::thread::join() >>
		//	 and collect all the 'exec_time_i' from each joining thread
		//   << consider using std::future for retrieving 'exec_time_i' after the thread finishes its do_work>>
		// - once all the threads have joined, the function "balance" must be called

		// Make sure evey invocation of the "balance" function returns 100000.
double parallel_do_work(Bank &bank, int iterations, int num_threads) {
	//Make the map
	create_map(bank);
	//Create threads, vector for execution times, and workloads
	std::vector<std::thread> threads;
	std::vector<double> exec_times(num_threads);
	int work_per_thread = iterations / num_threads;

	//Create threads and do work
	for (int i = 0; i < num_threads; i++) {
		threads.push_back(std::thread([&bank, &exec_times, i, work_per_thread]() {
			exec_times[i] = do_work(bank, work_per_thread);
		}));
	}
	//Join threads after execution to collect times 
	for (size_t i = 0; i < threads.size(); i++) {
		threads[i].join();
	}
	//final sanity check (there seems to be rounding issues)
	cout << "Final balance: " << balance(bank) << endl;
	//Return the max time (throughput of the slowest thread)
	double max_time = *max_element(exec_times.begin(), exec_times.end());
	return max_time;
}


// Step 7
		// Now configure your application to perform the SAME TOTAL amount
		// of iterations just executed, but all done by a single thread.
		// Measure the time to perform them and compare with the time
		// previously collected.
		// Which conclusion can you draw?
		// Which optimization can you do to the single-threaded execution in
		// order to improve its performance?
double sequential_do_work(Bank &bank, int iterations) {
	//Make the map and sequentially execute everything 
	create_map(bank);
	double exec_time = do_work(bank, iterations);
	//also sanity check
	cout << "Final balance: " << balance(bank) << endl;
	return exec_time;
}
// Step 8
		// Remove all the items in the map by leveraging the 'remove' function of the map
		// Destroy all the allocated resources (if any)
		// Execution terminates.
		// If you reach this stage happy, then you did a good job!
void remove_map(Bank &bank) {
	//Clear the map
	bank.accounts.clear();
}

// Final step
	// Produce plot
	// I expect each submission to include at least one plot in which
	// the x-axis is the concurrent threads used {1;2;4;8}
	// the y-axis is the application execution time.
	// The performance at 1 thread must be the sequential
	// application without synchronization primitives
//Main to run (I am going to test more this weekend and try one more solution and plot it)
int main() {
	//Create the bank
	Bank bank;

	//Run the parallel work with different thread counts
	vector<int> thread_counts = {1, 2, 4, 8, 16, 24};
	for (int num_threads : thread_counts) {
		double max_parallel_time = parallel_do_work(bank, 5000000, num_threads);
		cout << "Max parallel execution time with " << num_threads << " threads: " << max_parallel_time << " seconds" << endl;
	}
	//Run the sequential work
	double sequential_time = sequential_do_work(bank, 5000000);
	cout << "Sequential execution time: " << sequential_time << " seconds" << endl;

	remove_map(bank);
	return 0;
	//TODO: Make script that exports results to a graph 
}