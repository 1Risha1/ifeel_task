#include <iostream>
#include <vector>

using std::vector;


class number_decompozitor {
public: 
	number_decompozitor(int max_N) {
		vector<int> primes;

		min_divisors.resize(max_N + 1);
		min_divisors[1] = 1;

		for (int i = 2; i <= max_N; ++i) {
			if (min_divisors[i] == 0) {
				min_divisors[i] = i;
				primes.push_back(i);
			}
			for (int j = 0; j < (int)primes.size() && primes[j] <= min_divisors[i] && i*primes[j] <= max_N; ++j) {
				min_divisors[i * primes[j]] = primes[j];
			}
		}

		
	}

	void print_number_decomposition(unsigned n) {
		do {
			std::cout << min_divisors[n] << ' ';
			n /= min_divisors[n];
		} while (n != 1);
	}

private:
	vector<int> min_divisors;
};


int main() {
	int N;
	
	std::cin >> N;
	number_decompozitor dec(N);

	for (int i = 1; i <= N; i++) {
		dec.print_number_decomposition(i);
		std::cout << std::endl;
	}
	return 0;
}
