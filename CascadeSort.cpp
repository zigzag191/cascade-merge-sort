#include "CascadeSort.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <exception>
#include <vector>
#include <string>
#include <cassert>

struct NumberStream
{
	NumberStream(const std::string& path, bool shouldClear = false)
		: fileName{ path }
		, buffer{ 0 }
	{
		std::ios::openmode mode = std::ios::in | std::ios::out;
		mode |= shouldClear ? std::ios::trunc : std::ios::app;
		file.open(fileName, mode);
		if (!shouldClear)
		{
			file << '\n';
			Rewind();
		}
	}

	int Get()
	{
		if (file.eof())
		{
			throw std::runtime_error{ "eof" };
		}
		int last = buffer;
		file >> buffer;
		return last;
	}

	int Peek()
	{
		if (file.eof())
		{
			throw std::runtime_error{ "eof" };
		}
		return buffer;
	}

	void Put(int value)
	{
		file << value << '\n';
	}

	void Rewind()
	{
		file.clear();
		file.seekp(0);
		file.seekg(0);
		file >> buffer;
	}

	void Clear()
	{
		file.close();
		file.open(fileName, std::ios::in | std::ios::out | std::ios::trunc);
	}

	std::string fileName;
	std::fstream file;
	int buffer;
};

struct SortState
{
	std::vector<NumberStream> temporaryFiles;
	std::vector<std::size_t> nextDistribution;
	std::vector<std::size_t> fake;
	std::vector<std::size_t> logicalTapes;
	std::vector<std::size_t> seriesCount;
	std::size_t level = 0;
};

static void CheckFiles(SortState& state)
{
	for (auto& f : state.temporaryFiles)
	{
		assert(!f.file.eof());
	}
}

static void PrintState(SortState& state, bool printFake = false, const std::vector<std::size_t>* m = nullptr)
{
	std::cout << "\nКоличестов действительных серий: ";
	for (std::size_t i = 0; i < state.seriesCount.size(); ++i)
	{
		std::cout << std::setw(3) << state.seriesCount[state.logicalTapes[i]] << ' ';
	}
	if (printFake)
	{
		std::cout << "\nКоличество фиктивных серий:      ";
		for (std::size_t i = 0; i < state.fake.size(); ++i)
		{
			std::cout << std::setw(3) << state.fake[i] << ' ';
		}
	}
	if (m != nullptr)
	{
		std::cout << "\nКоличество макс. серий:          ";
		for (std::size_t i = 0; i < state.fake.size(); ++i)
		{
			std::cout << std::setw(3) << (*m)[i] << ' ';
		}
	}
	std::cout << '\n';
	std::cin.get();
}

static void PrintDistribution(SortState& state, const std::vector<std::size_t>& lastDistribution)
{
	std::cout << "\nДостигнутое распределение:       ";
	for (std::size_t i = 0; i < state.seriesCount.size(); ++i)
	{
		std::cout << std::setw(3) << lastDistribution[i] << ' ';
	}
	std::cout << "\nСледующее распределение:         ";
	for (std::size_t i = 0; i < state.seriesCount.size(); ++i)
	{
		std::cout << std::setw(3) << state.nextDistribution[i] << ' ';
	}
	PrintState(state, true);
}

using Iterator = std::vector<NumberStream>::iterator;
static void MergeOne(const std::vector<Iterator>& from, NumberStream& to)
{
	std::vector<int> inputBuffer(from.size());
	std::vector<bool> stringIsValid(from.size(), false);

	for (std::size_t i = 0; i < from.size(); ++i)
	{
		NumberStream& file = *from[i];
		if (!file.file.eof())
		{
			inputBuffer[i] = file.Get();
			stringIsValid[i] = true;
		}
	}

	for (;;)
	{
		int max = -1;
		for (int i = 0; i < from.size(); ++i)
		{
			if (stringIsValid[i])
			{
				if (max == -1)
				{
					max = i;
				}
				else if (inputBuffer[max] < inputBuffer[i])
				{
					max = i;
				}
			}
		}

		if (max == -1)
		{
			return;
		}

		to.Put(inputBuffer[max]);

		NumberStream& maxFile = *from[max];
		if (maxFile.file.eof() || maxFile.Peek() > inputBuffer[max])
		{
			stringIsValid[max] = false;
		}
		else
		{
			inputBuffer[max] = maxFile.Get();
		}
	}
}

static void CopyRun(NumberStream& from, NumberStream& to, int& last)
{
	int prev = from.Get();
	to.Put(prev);
	while (!from.file.eof() && prev >= from.Peek())
	{
		prev = from.Get();
		to.Put(prev);
	}
	last = prev;
}

static void Distribute(SortState& state, NumberStream& inputFile)
{
	auto size = state.temporaryFiles.size();

	std::vector<std::size_t> lastDistribution(size, 0);

	state.nextDistribution.resize(size, 0);
	state.nextDistribution[0] = 1;

	state.fake.resize(size, 0);

	state.logicalTapes.resize(size, 0);
	for (std::size_t i = 0; i < size; ++i)
	{
		state.logicalTapes[i] = i;
	}

	std::vector<int> last(size);
	CopyRun(inputFile, state.temporaryFiles[state.logicalTapes[0]], last[state.logicalTapes[0]]);

	state.seriesCount.resize(size, 0);
	state.seriesCount[state.logicalTapes[0]] = 1;

	state.level = 0;

	PrintDistribution(state, lastDistribution);
	while (!inputFile.file.eof())
	{
		++state.level;

		for (std::size_t i = 0; i < size; ++i)
		{
			lastDistribution[i] = state.nextDistribution[i];
		}
		for (std::size_t i = 0; i < size - 1; ++i)
		{
			state.nextDistribution[size - i - 2] = state.nextDistribution[size - i - 1] + lastDistribution[i];
		}
		for (std::size_t left = 0, right = size - 2; left < right; ++left, --right)
		{
			std::swap(state.logicalTapes[left], state.logicalTapes[right]);
		}
		for (std::size_t i = 0; i < size - 1; ++i)
		{
			state.fake[i] = state.nextDistribution[i + 1];
		}
		PrintDistribution(state, lastDistribution);
		bool hasMoreData = true;
		for (int fi = 0; fi < size - 2 && hasMoreData; ++fi)
		{
			for (int fj = fi; fj >= 0 && hasMoreData; --fj)
			{
				int currentFile = fj;
				std::size_t seriesToPut = lastDistribution[size - fj - 3];

				while (currentFile != -1 && hasMoreData)
				{
					while (seriesToPut != 0)
					{
						std::size_t physicalIndex = state.logicalTapes[currentFile];
						if (state.level > 1 && !inputFile.file.eof() && last[physicalIndex] >= inputFile.Peek())
						{
							CopyRun(inputFile, state.temporaryFiles[physicalIndex], last[physicalIndex]);
						}
						if (inputFile.file.eof())
						{
							hasMoreData = false;
							break;
						}
						if (!inputFile.file.eof())
						{
							CopyRun(inputFile, state.temporaryFiles[physicalIndex], last[physicalIndex]);
						}
						++state.seriesCount[physicalIndex];
						--state.fake[currentFile];
						--seriesToPut;
						PrintDistribution(state, lastDistribution);
					}

					--currentFile;
					seriesToPut = lastDistribution[size - fj - 3] - lastDistribution[size - fj - 2];
				}
			}
		}
	}

	for (std::size_t i = 0; i < size - 1; ++i)
	{
		state.temporaryFiles[i].Rewind();
	}
}

static void EndCascade(SortState& state)
{
	state.temporaryFiles[state.logicalTapes[1]].Clear();
	std::swap(state.logicalTapes[0], state.logicalTapes[1]);
	--state.level;
	for (std::size_t left = 0, right = state.logicalTapes.size() - 1; left < right; ++left, --right)
	{
		std::swap(state.logicalTapes[left], state.logicalTapes[right]);
	}
}

static void FirstCascase(SortState& state)
{
	auto size = state.temporaryFiles.size();
	std::vector<std::size_t> maximum(size, 0);

	for (std::size_t i = 0; i < size - 1; ++i)
	{
		maximum[i] = state.nextDistribution[i + 1];
	}

	PrintState(state, true, &maximum);

	for (std::size_t i = size - 2; i > 0; --i)
	{
		std::size_t physicalIndex = state.logicalTapes[i];
		NumberStream& currentFile = state.temporaryFiles[physicalIndex];

		if (state.fake[i - 1] == maximum[i - 1])
		{
			std::swap(state.logicalTapes[i], state.logicalTapes[i + 1]);
			state.temporaryFiles[state.logicalTapes[i]].Clear();
			for (std::size_t j = 0; j < i; ++j)
			{
				state.fake[j] -= maximum[i - 1];
				maximum[j] -= maximum[i - 1];
			}
			PrintState(state, true, &maximum);
			continue;
		}

		for (std::size_t j = 0; j < i; ++j)
		{
			maximum[j] -= maximum[i - 1];
		}

		while (!currentFile.file.eof())
		{
			std::vector<Iterator> from;
			std::size_t last = state.logicalTapes[i + 1];
			for (std::size_t j = 0; j <= i; ++j)
			{
				if (state.fake[j] <= maximum[j])
				{
					std::size_t phisicalFrom = state.logicalTapes[j];
					from.push_back(state.temporaryFiles.begin() + phisicalFrom);
					--state.seriesCount[phisicalFrom];
				}
			}

			MergeOne(from, state.temporaryFiles[last]);
			++state.seriesCount[last];
			PrintState(state, true, &maximum);

			for (std::size_t j = 0; j <= i; ++j)
			{
				if (state.fake[j] > maximum[j])
				{
					--state.fake[j];
				}
			}
		}

		currentFile.Clear();
		state.temporaryFiles[state.logicalTapes[i + 1]].Rewind();
	}
	EndCascade(state);
	PrintState(state, true, &maximum);
}

static void Cascade(SortState& state)
{
	auto size = state.temporaryFiles.size();

	for (std::size_t i = size - 2; i > 0; --i)
	{
		std::size_t physicalIndex = state.logicalTapes[i];
		auto& currentFile = state.temporaryFiles[physicalIndex];

		while (!currentFile.file.eof())
		{
			std::vector<Iterator> from;
			std::size_t last = state.logicalTapes[i + 1];
			for (std::size_t j = 0; j <= i; ++j)
			{
				std::size_t phisicalFrom = state.logicalTapes[j];
				from.push_back(state.temporaryFiles.begin() + phisicalFrom);
				--state.seriesCount[phisicalFrom];
			}

			MergeOne(from, state.temporaryFiles[last]);
			++state.seriesCount[last];
			PrintState(state);
		}
		currentFile.Clear();
		state.temporaryFiles[state.logicalTapes[i + 1]].Rewind();
	}
	EndCascade(state);
	PrintState(state);
}

static void LastCascase(SortState& state, NumberStream& outputFile)
{
	auto size = state.logicalTapes.size();
	std::vector<Iterator> from;
	for (std::size_t j = 0; j < size - 1; ++j)
	{
		std::size_t phisicalFrom = state.logicalTapes[j];
		from.push_back(state.temporaryFiles.begin() + phisicalFrom);
		--state.seriesCount[phisicalFrom];
	}
	MergeOne(from, outputFile);
	PrintState(state);
}

void CascadeSort(std::string_view fileName, std::size_t numberOfFiles)
{
	if (numberOfFiles < 3)
	{
		throw std::runtime_error{ "at least 3 temporary files are required" };
	}

	SortState state;

	NumberStream inputFile{ fileName.data() };
	if (!inputFile.file.is_open())
	{
		throw std::runtime_error{ "could not open file" };
	}
	if (inputFile.file.eof())
	{
		return;
	}

	std::string baseFileName = "tmp";
	for (std::size_t i = 0; i < numberOfFiles; ++i)
	{
		state.temporaryFiles.emplace_back(baseFileName + std::to_string(i), true);
	}

	std::cout << "*********** Фаза распределения ***********\n";
	Distribute(state, inputFile);

	std::cout << "*********** Фаза слияния ***********\n";
	if (state.level > 1)
	{
		FirstCascase(state);
		while (state.level != 1)
		{
			Cascade(state);
		}
	}

	inputFile.Clear();
	LastCascase(state, inputFile);
}
