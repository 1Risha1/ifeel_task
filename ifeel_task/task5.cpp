#include <map>
#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <istream>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <boost\atomic\atomic.hpp>

using namespace std;
#define EPSILON 4*10e-3

typedef  unsigned int PositionIndex;

class spinlock {
	std::atomic_flag _lock;
public:
	void lock() {
		while (_lock.test_and_set(std::memory_order_acquire))  // acquire lock
			; // spin
	}
	void unlock() {
		_lock.clear(std::memory_order_release);               // release lock
	}

};

struct pair_sent {
	vector<PositionIndex> english_sentence;
	vector<PositionIndex> foreign_sentence;

	pair_sent(vector<PositionIndex> sent_e, vector<PositionIndex> sent_f) : english_sentence(sent_e), foreign_sentence(sent_f) {};

	size_t size_eng() const {
		return english_sentence.size();
	}

	size_t size_foreign() const {
		return foreign_sentence.size();
	}
};

struct Vocabulary {
	map<string, PositionIndex> english, foreign;
	vector<string> english_, foreign_;
};

class pr_t {
	vector<unordered_map<PositionIndex, double>> data;
	friend class Iterator;
	double p;
	vector<spinlock> spins;

	class Iterator {
		int i;
		pr_t* table;

	public:
		Iterator(int _i, pr_t *t) :i(_i), table(t) {}
		double& operator[] (int j) {
			lock_guard<spinlock> guard(table->spins[i]);
			if (table->data[i].find(j) == table->data[i].end()) {
				table->data[i][j] = table->p;
			}
			return table->data[i][j];
		}
	};

public:
	pr_t(double _p = 0.5) : p(_p) {}

	pr_t(size_t size, double _p = 0.5) : p(_p), data(size), spins(size) {}

	Iterator operator[] (int i) {
		return Iterator(i, this);
	}

	void resize(size_t new_size) {
		data.resize(new_size);
		spins.resize(new_size);
	}
};

class parralel_sentences_t {

	vector<pair_sent> data;
	Vocabulary voc;
	pr_t probability_table;
	vector<spinlock> spins;
	vector<spinlock> spins_f;

	// по номеру слова из английского предложения находит соответствующее слово
	vector<vector<int>> aligment;

	void init() {
		probability_table = pr_t(voc.english.size(), double(1.0) / voc.foreign.size());
		aligment_init();
		count_init();
		total_init();
		spins.resize(voc.english_.size());
		spins_f.resize(voc.foreign_.size());
	}

	void aligment_init() {
		aligment.resize(data.size());
		for (size_t i = 0; i < aligment.size(); ++i) {
			aligment[i].resize(data[i].size_eng());
		}
	}

public:

	vector<unordered_map<PositionIndex, double>> _count;
	vector<double> _total;

	void total_init() {
		_total.clear();
		_total.resize(get_foreign_voc_size(), 0);
	}

	void count_init() {
		_count.clear();
		_count.resize(get_english_voc_size());
	}


	int get_english_voc_size() const {
		return voc.english.size();
	}

	int get_foreign_voc_size() const {
		return voc.foreign.size();
	}

	const string& get_english_word(PositionIndex e) const {
		return voc.english_[e];
	}

	const string& get_foreign_word(PositionIndex f) const {
		return voc.foreign_[f];
	}

	const pair_sent& operator[] (int i) const {
		return data[i];
	}

	void push(pair_sent &sents) {
		data.push_back(sents);
	}

	size_t size() {
		return data.size();
	}

	// возврващает верояность перевода e как f, e и f - номера слов в i-ом предложении
	double& operator() (int i, PositionIndex e, PositionIndex f) {
		return probability_table[data[i].english_sentence[e]][data[i].foreign_sentence[f]];
	}

	// возвращает вероятность, где e и f позиции слов в словаре
	double& operator() (PositionIndex e, PositionIndex f) {
		return probability_table[e][f];
	}

	double& count(int i, PositionIndex e, PositionIndex f) {
		return _count[data[i].english_sentence[e]][data[i].foreign_sentence[f]];
	}

	void count_inc(int i, PositionIndex e, PositionIndex f, double p) {
		PositionIndex eng = data[i].english_sentence[e];
		PositionIndex foreign = data[i].foreign_sentence[f];
		lock_guard<spinlock> guard(spins[eng]);
		_count[eng][foreign] += p;
	}

	void total_inc(int i, PositionIndex f, double p) {
		PositionIndex foreign = data[i].foreign_sentence[f];
		lock_guard<spinlock> guard(spins_f[foreign]);
		_total[foreign] += p;
	}

	double& count(PositionIndex e, PositionIndex f) {
		return _count[e][f];
	}

	double& total(int i, PositionIndex f) {
		return _total[data[i].foreign_sentence[f]];
	}

	double& total(PositionIndex f) {
		return _total[f];
	}

	void set_aligment(int i, PositionIndex e, PositionIndex f) {
		aligment[i][e] = f;
	}

	friend ostream& operator<< (ostream& o, parralel_sentences_t &data);
	friend istream& operator>> (istream &i, parralel_sentences_t &t);
};

ostream& operator<< (ostream& o, parralel_sentences_t &t) {
	for (size_t i = 0; i < t.size(); i++) {
		for (size_t j = 0; j < t[i].foreign_sentence.size(); ++j) {
			o << t.get_foreign_word(t.data[i].foreign_sentence[j]) << " ";
		}
		o << endl;
		for (size_t j = 0; j < t[i].english_sentence.size(); ++j) {
			o << t.get_english_word(t.data[i].english_sentence[j]) << " ";
		}
		o << endl;
		for (size_t j = 0; j < t.aligment[i].size(); j++) {
			// j - номер английского слова в i-ом предложении
			o.precision(2);
			o << t.aligment[i][j] << "-" << j << "(p=" << t(i, j, t.aligment[i][j]) << "%)  ";
		}
		o << endl;
	}
	return o;
}

istream& operator>> (istream &i, parralel_sentences_t &t) {
	string line;

	// показывает чило слов, находящихся в соответствующих словарях
	PositionIndex english_count = 0, foreign_count = 0;

	while (getline(i, line)) {

		vector <PositionIndex> english_sentene, foreign_sentence;
		string word;
		istringstream foreign_sentence_stream(line);

		while (foreign_sentence_stream >> word) {
			// ищем слово в словаре
			if (t.voc.foreign.find(word) == t.voc.foreign.end()) {
				t.voc.foreign[word] = foreign_count;
				t.voc.foreign_.push_back(word);
				foreign_sentence.push_back(foreign_count);
				foreign_count++;
			}
			else {
				// предложение содержит номер слова в словаре
				foreign_sentence.push_back(t.voc.foreign[word]);
			}
		}

		getline(i, line);
		istringstream english_sentence_stream(line);

		while (english_sentence_stream >> word) {
			if (t.voc.english.find(word) == t.voc.english.end()) {
				t.voc.english[word] = english_count;
				t.voc.english_.push_back(word);
				english_sentene.push_back(english_count);
				english_count++;
			}
			else {
				english_sentene.push_back(t.voc.english[word]);
			}
		}

		//  в следующей строке записано выравнивание, её пропускаем
		getline(i, line);

		t.push(pair_sent(move(english_sentene), move(foreign_sentence)));
	}

	// после прочтения, инициализируем таблицу вероятностей и выравнивание
	t.init();

	return i;
}

void thread_function(int l, int m, parralel_sentences_t &t) {

	for (int i = l; i < m; ++i) {
		pair_sent e_f = t[i];

		vector <double> total_s(e_f.size_eng());
		for (size_t e = 0; e < e_f.size_eng(); e++) {
			total_s[e] = 0;
			for (size_t f = 0; f < e_f.size_foreign(); ++f) {
				total_s[e] += t(i, e, f);
			}
		}
		for (size_t e = 0; e < e_f.size_eng(); e++) {
			for (size_t f = 0; f < e_f.size_foreign(); ++f) {
				t.count_inc(i, e, f, t(i, e, f) / total_s[e]);
				t.total_inc(i, f, t(i, e, f) / total_s[e]);
			}
		}
	}
}

void count_probs(parralel_sentences_t &t, atomic<bool>& convergenced, int l, int m) {

	for (int e = l; e < m; e++) {
		for (unordered_map<PositionIndex, double>::iterator f = t._count[e].begin(); f != t._count[e].end(); ++f) {
			if (f->second / t.total(f->first) > 10e-7) {
				double prev = t(e, f->first);
				t(e, f->first) = f->second / t.total(f->first);
				if (abs(t(e, f->first) - prev) > EPSILON) {
					convergenced = false;
				}
			}
		}
	}
}

void em(parralel_sentences_t &t, int thread_number = 4) {

	atomic<bool> convergenced = true;
	int k = 0;
	do {
		t.count_init();
		t.total_init();
		auto t1 = chrono::high_resolution_clock::now();
		convergenced = true;

		int block_size = t.size() / thread_number;
		vector<thread> threads(thread_number - 1);
		for (int i = 0; i < thread_number - 1; ++i) {
			threads[i] = thread(thread_function, block_size * i, block_size * (i + 1), ref(t));
		}
		thread_function(block_size * (thread_number - 1), t.size(), ref(t));
		for (int i = 0; i < thread_number - 1; ++i) {
			threads[i].join();
		}


		block_size = t.get_english_voc_size() / thread_number;
		for (int i = 0; i < thread_number - 1; ++i) {
			threads[i] = thread(count_probs, ref(t), ref(convergenced), block_size * i, block_size * (i + 1));
		}
		count_probs(ref(t), ref(convergenced), block_size * (thread_number - 1), t.get_english_voc_size());
		for (int i = 0; i < thread_number - 1; ++i) {
			threads[i].join();
		}

		k++;
		cout << k << "-s iteration takes " << chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - t1).count() << "ms" << endl;
	} while (!convergenced);


	for (size_t i = 0; i < t.size(); ++i) {
		for (size_t e = 0; e < t[i].size_eng(); e++) {
			double max_t_f = 0;
			for (size_t f = 0; f < t[i].size_foreign(); ++f) {
				if (max_t_f < t(i, e, f)) {
					max_t_f = t(i, e, f);
					t.set_aligment(i, e, f);
				}
			}
		}
	}
}

int main() {
	parralel_sentences_t s;
	ifstream fin("alignment-de-en.txt");
	ofstream fout("output.txt");
	fin >> s;
	em(s);
	fout << s;
}