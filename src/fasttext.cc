/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "fasttext.h"
#include "loss.h"
#include "quantmatrix.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fasttext {

constexpr int32_t FASTTEXT_VERSION = 12; /* Version 1b */
constexpr int32_t FASTTEXT_FILEFORMAT_MAGIC_INT32 = 793712314;


std::shared_ptr<Loss> FastText::createLoss(std::shared_ptr<Matrix>& output) {
  loss_name lossName = args_->loss;
  switch (lossName) {
    case loss_name::hs:
      return std::make_shared<HierarchicalSoftmaxLoss>(
          output, getTargetCounts());
    case loss_name::ns:
      return std::make_shared<NegativeSamplingLoss>(
          output, args_->neg, getTargetCounts());
    case loss_name::softmax:
      return std::make_shared<SoftmaxLoss>(output);
    case loss_name::ova:
      return std::make_shared<OneVsAllLoss>(output);
    default:
      throw std::runtime_error("Unknown loss");
  }
}

bool FastText::checkModel(std::istream& in) {
  int32_t magic;
  in.read((char*)&(magic), sizeof(int32_t));
  if (magic != FASTTEXT_FILEFORMAT_MAGIC_INT32) {
    return false;
  }
  in.read((char*)&(version), sizeof(int32_t));
  if (version > FASTTEXT_VERSION) {
    return false;
  }
  return true;
}

void FastText::loadModel(const std::string& filename) {
  std::ifstream ifs(filename, std::ifstream::binary);
  if (!ifs.is_open()) {
    throw std::invalid_argument(filename + " cannot be opened for loading!");
  }
  if (!checkModel(ifs)) {
    throw std::invalid_argument(filename + " has wrong file format!");
  }
  loadModel(ifs);
  ifs.close();
}

std::vector<int64_t> FastText::getTargetCounts() const {
  if (args_->model == model_name::sup) {
    return dict_->getCounts(entry_type::label);
  } else {
    return dict_->getCounts(entry_type::word);
  }
}

void FastText::buildModel() {
  auto loss = createLoss(output_);
  bool normalizeGradient = (args_->model == model_name::sup);
  model_ = std::make_shared<Model>(input_, output_, loss, normalizeGradient);
}

void FastText::loadModel(std::istream& in) {
  args_ = std::make_shared<Args>();
  input_ = std::make_shared<DenseMatrix>();
  output_ = std::make_shared<DenseMatrix>();
  args_->load(in);
  if (version == 11 && args_->model == model_name::sup) {
    // backward compatibility: old supervised models do not use char ngrams.
    args_->maxn = 0;
  }
  dict_ = std::make_shared<Dictionary>(args_, in);

  bool quant_input;
  in.read((char*)&quant_input, sizeof(bool));
  if (quant_input) {
    quant_ = true;
    input_ = std::make_shared<QuantMatrix>();
  }
  input_->load(in);

  if (!quant_input && dict_->isPruned()) {
    throw std::invalid_argument(
        "Invalid model file.\n"
        "Please download the updated model from www.fasttext.cc.\n"
        "See issue #332 on Github for more information.\n");
  }

  in.read((char*)&args_->qout, sizeof(bool));
  if (quant_ && args_->qout) {
    output_ = std::make_shared<QuantMatrix>();
  }
  output_->load(in);

  buildModel();
}

void FastText::predict(
    int32_t k,
    const std::vector<int32_t>& words,
    Predictions& predictions,
    real threshold) const {
  if (words.empty()) {
    return;
  }
  Model::State state(args_->dim, dict_->nlabels(), 0);
  if (args_->model != model_name::sup) {
    throw std::invalid_argument("Model needs to be supervised for prediction!");
  }
  model_->predict(words, k, threshold, predictions, state);
}

bool FastText::predictLine(
    std::istream& in,
    std::vector<std::pair<real, std::string>>& predictions,
    int32_t k,
    real threshold) const {
  predictions.clear();
  if (in.peek() == EOF) {
    return false;
  }

  std::vector<int32_t> words, labels;
  dict_->getLine(in, words, labels);
  Predictions linePredictions;
  predict(k, words, linePredictions, threshold);
  for (const auto& p : linePredictions) {
    predictions.push_back(
        std::make_pair(std::exp(p.first), dict_->getLabel(p.second)));
  }

  return true;
}

<<<<<<< HEAD
void FastText::getSentenceVector(std::istream& in, fasttext::Vector& svec) {
  svec.zero();
  if (args_->model == model_name::sup) {
    std::vector<int32_t> line, labels;
    dict_->getLine(in, line, labels);
    for (int32_t i = 0; i < line.size(); i++) {
      addInputVector(svec, line[i]);
    }
    if (!line.empty()) {
      svec.mul(1.0 / line.size());
    }
  } else {
    Vector vec(args_->dim);
    std::string sentence;
    std::getline(in, sentence);
    std::istringstream iss(sentence);
    std::string word;
    int32_t count = 0;
    while (iss >> word) {
      getWordVector(vec, word);
      real norm = vec.norm();
      if (norm > 0) {
        vec.mul(1.0 / norm);
        svec.addVector(vec);
        count++;
      }
    }
    if (count > 0) {
      svec.mul(1.0 / count);
    }
  }
}

std::vector<std::pair<std::string, Vector>> FastText::getNgramVectors(
    const std::string& word) const {
  std::vector<std::pair<std::string, Vector>> result;
  std::vector<int32_t> ngrams;
  std::vector<std::string> substrings;
  dict_->getSubwords(word, ngrams, substrings);
  assert(ngrams.size() <= substrings.size());
  for (int32_t i = 0; i < ngrams.size(); i++) {
    Vector vec(args_->dim);
    if (ngrams[i] >= 0) {
      vec.addRow(*input_, ngrams[i]);
    }
    result.emplace_back(substrings[i], std::move(vec));
  }
  return result;
}

void FastText::precomputeWordVectors(DenseMatrix& wordVectors) {
  Vector vec(args_->dim);
  wordVectors.zero();
  for (int32_t i = 0; i < dict_->nwords(); i++) {
    std::string word = dict_->getWord(i);
    getWordVector(vec, word);
    real norm = vec.norm();
    if (norm > 0) {
      wordVectors.addVectorToRow(vec, i, 1.0 / norm);
    }
  }
}

void FastText::lazyComputeWordVectors() {
  if (!wordVectors_) {
    wordVectors_ = std::unique_ptr<DenseMatrix>(
        new DenseMatrix(dict_->nwords(), args_->dim));
    precomputeWordVectors(*wordVectors_);
  }
}

std::vector<std::pair<real, std::string>> FastText::getNN(
    const std::string& word,
    int32_t k) {
  Vector query(args_->dim);

  getWordVector(query, word);

  lazyComputeWordVectors();
  assert(wordVectors_);
  return getNN(*wordVectors_, query, k, {word});
}

std::vector<std::pair<real, std::string>> FastText::getNN(
    const DenseMatrix& wordVectors,
    const Vector& query,
    int32_t k,
    const std::set<std::string>& banSet) {
  std::vector<std::pair<real, std::string>> heap;

  real queryNorm = query.norm();
  if (std::abs(queryNorm) < 1e-8) {
    queryNorm = 1;
  }

  for (int32_t i = 0; i < dict_->nwords(); i++) {
    std::string word = dict_->getWord(i);
    if (banSet.find(word) == banSet.end()) {
      real dp = wordVectors.dotRow(query, i);
      real similarity = dp / queryNorm;

      if (heap.size() == k && similarity < heap.front().first) {
        continue;
      }
      heap.push_back(std::make_pair(similarity, word));
      std::push_heap(heap.begin(), heap.end(), comparePairs);
      if (heap.size() > k) {
        std::pop_heap(heap.begin(), heap.end(), comparePairs);
        heap.pop_back();
      }
    }
  }
  std::sort_heap(heap.begin(), heap.end(), comparePairs);

  return heap;
}

std::vector<std::pair<real, std::string>> FastText::getAnalogies(
    int32_t k,
    const std::string& wordA,
    const std::string& wordB,
    const std::string& wordC) {
  Vector query(args_->dim);
  query.zero();

  Vector buffer(args_->dim);
  getWordVector(buffer, wordA);
  query.addVector(buffer, 1.0 / (buffer.norm() + 1e-8));
  getWordVector(buffer, wordB);
  query.addVector(buffer, -1.0 / (buffer.norm() + 1e-8));
  getWordVector(buffer, wordC);
  query.addVector(buffer, 1.0 / (buffer.norm() + 1e-8));

  lazyComputeWordVectors();
  assert(wordVectors_);
  return getNN(*wordVectors_, query, k, {wordA, wordB, wordC});
}

bool FastText::keepTraining(const int64_t ntokens) const {
  return tokenCount_ < args_->epoch * ntokens && !trainException_;
}

void FastText::trainThread(int32_t threadId, const TrainCallback& callback) {
  std::ifstream ifs(args_->input);
  utils::seek(ifs, threadId * utils::size(ifs) / args_->thread);

  Model::State state(args_->dim, output_->size(0), threadId + args_->seed);

  const int64_t ntokens = dict_->ntokens();
  int64_t localTokenCount = 0;
  std::vector<int32_t> line, labels;
  uint64_t callbackCounter = 0;
  try {
    while (keepTraining(ntokens)) {
      real progress = real(tokenCount_) / (args_->epoch * ntokens);
      if (callback && ((callbackCounter++ % 64) == 0)) {
        double wst;
        double lr;
        int64_t eta;
        std::tie<double, double, int64_t>(wst, lr, eta) =
            progressInfo(progress);
        callback(progress, loss_, wst, lr, eta);
      }
      real lr = args_->lr * (1.0 - progress);
      if (args_->model == model_name::sup) {
        localTokenCount += dict_->getLine(ifs, line, labels);
        supervised(state, lr, line, labels);
      } else if (args_->model == model_name::cbow) {
        localTokenCount += dict_->getLine(ifs, line, state.rng);
        cbow(state, lr, line);
      } else if (args_->model == model_name::sg) {
        localTokenCount += dict_->getLine(ifs, line, state.rng);
        skipgram(state, lr, line);
      }
      if (localTokenCount > args_->lrUpdateRate) {
        tokenCount_ += localTokenCount;
        localTokenCount = 0;
        if (threadId == 0 && args_->verbose > 1) {
          loss_ = state.getLoss();
        }
      }
    }
  } catch (DenseMatrix::EncounteredNaNError&) {
    trainException_ = std::current_exception();
  }
  if (threadId == 0)
    loss_ = state.getLoss();
  ifs.close();
}

std::shared_ptr<Matrix> FastText::getInputMatrixFromFile(
    const std::string& filename) const {
  std::ifstream in(filename);
  std::vector<std::string> words;
  std::shared_ptr<DenseMatrix> mat; // temp. matrix for pretrained vectors
  int64_t n, dim;
  if (!in.is_open()) {
    throw std::invalid_argument(filename + " cannot be opened for loading!");
  }
  in >> n >> dim;
  if (dim != args_->dim) {
    throw std::invalid_argument(
        "Dimension of pretrained vectors (" + std::to_string(dim) +
        ") does not match dimension (" + std::to_string(args_->dim) + ")!");
  }
  mat = std::make_shared<DenseMatrix>(n, dim);
  for (size_t i = 0; i < n; i++) {
    std::string word;
    in >> word;
    words.push_back(word);
    dict_->add(word);
    for (size_t j = 0; j < dim; j++) {
      in >> mat->at(i, j);
    }
  }
  in.close();

  dict_->threshold(1, 0);
  dict_->init();
  std::shared_ptr<DenseMatrix> input = std::make_shared<DenseMatrix>(
      dict_->nwords() + args_->bucket, args_->dim);
  input->uniform(1.0 / args_->dim, args_->thread, args_->seed);

  for (size_t i = 0; i < n; i++) {
    int32_t idx = dict_->getId(words[i]);
    if (idx < 0 || idx >= dict_->nwords()) {
      continue;
    }
    for (size_t j = 0; j < dim; j++) {
      input->at(idx, j) = mat->at(i, j);
    }
  }
  return input;
}

std::shared_ptr<Matrix> FastText::createRandomMatrix() const {
  std::shared_ptr<DenseMatrix> input = std::make_shared<DenseMatrix>(
      dict_->nwords() + args_->bucket, args_->dim);
  input->uniform(1.0 / args_->dim, args_->thread, args_->seed);

  return input;
}

std::shared_ptr<Matrix> FastText::createTrainOutputMatrix() const {
  int64_t m =
      (args_->model == model_name::sup) ? dict_->nlabels() : dict_->nwords();
  std::shared_ptr<DenseMatrix> output =
      std::make_shared<DenseMatrix>(m, args_->dim);
  output->zero();

  return output;
}

void FastText::train(const Args& args, const TrainCallback& callback) {
  args_ = std::make_shared<Args>(args);
  dict_ = std::make_shared<Dictionary>(args_);
  if (args_->input == "-") {
    // manage expectations
    throw std::invalid_argument("Cannot use stdin for training!");
  }
  std::ifstream ifs(args_->input);
  if (!ifs.is_open()) {
    throw std::invalid_argument(
        args_->input + " cannot be opened for training!");
  }
  dict_->readFromFile(ifs);
  ifs.close();

  if (!args_->pretrainedVectors.empty()) {
    input_ = getInputMatrixFromFile(args_->pretrainedVectors);
  } else {
    input_ = createRandomMatrix();
  }
  output_ = createTrainOutputMatrix();
  quant_ = false;
  auto loss = createLoss(output_);
  bool normalizeGradient = (args_->model == model_name::sup);
  model_ = std::make_shared<Model>(input_, output_, loss, normalizeGradient);
  startThreads(callback);
}

void FastText::abort() {
  try {
    throw AbortError();
  } catch (AbortError&) {
    trainException_ = std::current_exception();
  }
}

void FastText::startThreads(const TrainCallback& callback) {
  start_ = std::chrono::steady_clock::now();
  tokenCount_ = 0;
  loss_ = -1;
  trainException_ = nullptr;
  std::vector<std::thread> threads;
  if (args_->thread > 1) {
    for (int32_t i = 0; i < args_->thread; i++) {
      threads.push_back(std::thread([=]() { trainThread(i, callback); }));
    }
  } else {
    // webassembly can't instantiate `std::thread`
    trainThread(0, callback);
  }
  const int64_t ntokens = dict_->ntokens();
  // Same condition as trainThread
  while (keepTraining(ntokens)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (loss_ >= 0 && args_->verbose > 1) {
      real progress = real(tokenCount_) / (args_->epoch * ntokens);
      std::cerr << "\r";
      printInfo(progress, loss_, std::cerr);
    }
  }
  for (int32_t i = 0; i < threads.size(); i++) {
    threads[i].join();
  }
  if (trainException_) {
    std::exception_ptr exception = trainException_;
    trainException_ = nullptr;
    std::rethrow_exception(exception);
  }
  if (args_->verbose > 0) {
    std::cerr << "\r";
    printInfo(1.0, loss_, std::cerr);
    std::cerr << std::endl;
  }
}

int FastText::getDimension() const {
  return args_->dim;
}

bool FastText::isQuant() const {
  return quant_;
}

bool comparePairs(
    const std::pair<real, std::string>& l,
    const std::pair<real, std::string>& r) {
  return l.first > r.first;
}

||||||| 3697152
void FastText::getSentenceVector(std::istream& in, fasttext::Vector& svec) {
  svec.zero();
  if (args_->model == model_name::sup) {
    std::vector<int32_t> line, labels;
    dict_->getLine(in, line, labels);
    for (int32_t i = 0; i < line.size(); i++) {
      addInputVector(svec, line[i]);
    }
    if (!line.empty()) {
      svec.mul(1.0 / line.size());
    }
  } else {
    Vector vec(args_->dim);
    std::string sentence;
    std::getline(in, sentence);
    std::istringstream iss(sentence);
    std::string word;
    int32_t count = 0;
    while (iss >> word) {
      getWordVector(vec, word);
      real norm = vec.norm();
      if (norm > 0) {
        vec.mul(1.0 / norm);
        svec.addVector(vec);
        count++;
      }
    }
    if (count > 0) {
      svec.mul(1.0 / count);
    }
  }
}

std::vector<std::pair<std::string, Vector>> FastText::getNgramVectors(
    const std::string& word) const {
  std::vector<std::pair<std::string, Vector>> result;
  std::vector<int32_t> ngrams;
  std::vector<std::string> substrings;
  dict_->getSubwords(word, ngrams, substrings);
  assert(ngrams.size() <= substrings.size());
  for (int32_t i = 0; i < ngrams.size(); i++) {
    Vector vec(args_->dim);
    if (ngrams[i] >= 0) {
      vec.addRow(*input_, ngrams[i]);
    }
    result.push_back(std::make_pair(substrings[i], std::move(vec)));
  }
  return result;
}

void FastText::precomputeWordVectors(DenseMatrix& wordVectors) {
  Vector vec(args_->dim);
  wordVectors.zero();
  for (int32_t i = 0; i < dict_->nwords(); i++) {
    std::string word = dict_->getWord(i);
    getWordVector(vec, word);
    real norm = vec.norm();
    if (norm > 0) {
      wordVectors.addVectorToRow(vec, i, 1.0 / norm);
    }
  }
}

void FastText::lazyComputeWordVectors() {
  if (!wordVectors_) {
    wordVectors_ = std::unique_ptr<DenseMatrix>(
        new DenseMatrix(dict_->nwords(), args_->dim));
    precomputeWordVectors(*wordVectors_);
  }
}

std::vector<std::pair<real, std::string>> FastText::getNN(
    const std::string& word,
    int32_t k) {
  Vector query(args_->dim);

  getWordVector(query, word);

  lazyComputeWordVectors();
  assert(wordVectors_);
  return getNN(*wordVectors_, query, k, {word});
}

std::vector<std::pair<real, std::string>> FastText::getNN(
    const DenseMatrix& wordVectors,
    const Vector& query,
    int32_t k,
    const std::set<std::string>& banSet) {
  std::vector<std::pair<real, std::string>> heap;

  real queryNorm = query.norm();
  if (std::abs(queryNorm) < 1e-8) {
    queryNorm = 1;
  }

  for (int32_t i = 0; i < dict_->nwords(); i++) {
    std::string word = dict_->getWord(i);
    if (banSet.find(word) == banSet.end()) {
      real dp = wordVectors.dotRow(query, i);
      real similarity = dp / queryNorm;

      if (heap.size() == k && similarity < heap.front().first) {
        continue;
      }
      heap.push_back(std::make_pair(similarity, word));
      std::push_heap(heap.begin(), heap.end(), comparePairs);
      if (heap.size() > k) {
        std::pop_heap(heap.begin(), heap.end(), comparePairs);
        heap.pop_back();
      }
    }
  }
  std::sort_heap(heap.begin(), heap.end(), comparePairs);

  return heap;
}

std::vector<std::pair<real, std::string>> FastText::getAnalogies(
    int32_t k,
    const std::string& wordA,
    const std::string& wordB,
    const std::string& wordC) {
  Vector query = Vector(args_->dim);
  query.zero();

  Vector buffer(args_->dim);
  getWordVector(buffer, wordA);
  query.addVector(buffer, 1.0 / (buffer.norm() + 1e-8));
  getWordVector(buffer, wordB);
  query.addVector(buffer, -1.0 / (buffer.norm() + 1e-8));
  getWordVector(buffer, wordC);
  query.addVector(buffer, 1.0 / (buffer.norm() + 1e-8));

  lazyComputeWordVectors();
  assert(wordVectors_);
  return getNN(*wordVectors_, query, k, {wordA, wordB, wordC});
}

bool FastText::keepTraining(const int64_t ntokens) const {
  return tokenCount_ < args_->epoch * ntokens && !trainException_;
}

void FastText::trainThread(int32_t threadId, const TrainCallback& callback) {
  std::ifstream ifs(args_->input);
  utils::seek(ifs, threadId * utils::size(ifs) / args_->thread);

  Model::State state(args_->dim, output_->size(0), threadId + args_->seed);

  const int64_t ntokens = dict_->ntokens();
  int64_t localTokenCount = 0;
  std::vector<int32_t> line, labels;
  uint64_t callbackCounter = 0;
  try {
    while (keepTraining(ntokens)) {
      real progress = real(tokenCount_) / (args_->epoch * ntokens);
      if (callback && ((callbackCounter++ % 64) == 0)) {
        double wst;
        double lr;
        int64_t eta;
        std::tie<double, double, int64_t>(wst, lr, eta) =
            progressInfo(progress);
        callback(progress, loss_, wst, lr, eta);
      }
      real lr = args_->lr * (1.0 - progress);
      if (args_->model == model_name::sup) {
        localTokenCount += dict_->getLine(ifs, line, labels);
        supervised(state, lr, line, labels);
      } else if (args_->model == model_name::cbow) {
        localTokenCount += dict_->getLine(ifs, line, state.rng);
        cbow(state, lr, line);
      } else if (args_->model == model_name::sg) {
        localTokenCount += dict_->getLine(ifs, line, state.rng);
        skipgram(state, lr, line);
      }
      if (localTokenCount > args_->lrUpdateRate) {
        tokenCount_ += localTokenCount;
        localTokenCount = 0;
        if (threadId == 0 && args_->verbose > 1) {
          loss_ = state.getLoss();
        }
      }
    }
  } catch (DenseMatrix::EncounteredNaNError&) {
    trainException_ = std::current_exception();
  }
  if (threadId == 0)
    loss_ = state.getLoss();
  ifs.close();
}

std::shared_ptr<Matrix> FastText::getInputMatrixFromFile(
    const std::string& filename) const {
  std::ifstream in(filename);
  std::vector<std::string> words;
  std::shared_ptr<DenseMatrix> mat; // temp. matrix for pretrained vectors
  int64_t n, dim;
  if (!in.is_open()) {
    throw std::invalid_argument(filename + " cannot be opened for loading!");
  }
  in >> n >> dim;
  if (dim != args_->dim) {
    throw std::invalid_argument(
        "Dimension of pretrained vectors (" + std::to_string(dim) +
        ") does not match dimension (" + std::to_string(args_->dim) + ")!");
  }
  mat = std::make_shared<DenseMatrix>(n, dim);
  for (size_t i = 0; i < n; i++) {
    std::string word;
    in >> word;
    words.push_back(word);
    dict_->add(word);
    for (size_t j = 0; j < dim; j++) {
      in >> mat->at(i, j);
    }
  }
  in.close();

  dict_->threshold(1, 0);
  dict_->init();
  std::shared_ptr<DenseMatrix> input = std::make_shared<DenseMatrix>(
      dict_->nwords() + args_->bucket, args_->dim);
  input->uniform(1.0 / args_->dim, args_->thread, args_->seed);

  for (size_t i = 0; i < n; i++) {
    int32_t idx = dict_->getId(words[i]);
    if (idx < 0 || idx >= dict_->nwords()) {
      continue;
    }
    for (size_t j = 0; j < dim; j++) {
      input->at(idx, j) = mat->at(i, j);
    }
  }
  return input;
}

std::shared_ptr<Matrix> FastText::createRandomMatrix() const {
  std::shared_ptr<DenseMatrix> input = std::make_shared<DenseMatrix>(
      dict_->nwords() + args_->bucket, args_->dim);
  input->uniform(1.0 / args_->dim, args_->thread, args_->seed);

  return input;
}

std::shared_ptr<Matrix> FastText::createTrainOutputMatrix() const {
  int64_t m =
      (args_->model == model_name::sup) ? dict_->nlabels() : dict_->nwords();
  std::shared_ptr<DenseMatrix> output =
      std::make_shared<DenseMatrix>(m, args_->dim);
  output->zero();

  return output;
}

void FastText::train(const Args& args, const TrainCallback& callback) {
  args_ = std::make_shared<Args>(args);
  dict_ = std::make_shared<Dictionary>(args_);
  if (args_->input == "-") {
    // manage expectations
    throw std::invalid_argument("Cannot use stdin for training!");
  }
  std::ifstream ifs(args_->input);
  if (!ifs.is_open()) {
    throw std::invalid_argument(
        args_->input + " cannot be opened for training!");
  }
  dict_->readFromFile(ifs);
  ifs.close();

  if (!args_->pretrainedVectors.empty()) {
    input_ = getInputMatrixFromFile(args_->pretrainedVectors);
  } else {
    input_ = createRandomMatrix();
  }
  output_ = createTrainOutputMatrix();
  quant_ = false;
  auto loss = createLoss(output_);
  bool normalizeGradient = (args_->model == model_name::sup);
  model_ = std::make_shared<Model>(input_, output_, loss, normalizeGradient);
  startThreads(callback);
}

void FastText::abort() {
  try {
    throw AbortError();
  } catch (AbortError&) {
    trainException_ = std::current_exception();
  }
}

void FastText::startThreads(const TrainCallback& callback) {
  start_ = std::chrono::steady_clock::now();
  tokenCount_ = 0;
  loss_ = -1;
  trainException_ = nullptr;
  std::vector<std::thread> threads;
  if (args_->thread > 1) {
    for (int32_t i = 0; i < args_->thread; i++) {
      threads.push_back(std::thread([=]() { trainThread(i, callback); }));
    }
  } else {
    // webassembly can't instantiate `std::thread`
    trainThread(0, callback);
  }
  const int64_t ntokens = dict_->ntokens();
  // Same condition as trainThread
  while (keepTraining(ntokens)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (loss_ >= 0 && args_->verbose > 1) {
      real progress = real(tokenCount_) / (args_->epoch * ntokens);
      std::cerr << "\r";
      printInfo(progress, loss_, std::cerr);
    }
  }
  for (int32_t i = 0; i < threads.size(); i++) {
    threads[i].join();
  }
  if (trainException_) {
    std::exception_ptr exception = trainException_;
    trainException_ = nullptr;
    std::rethrow_exception(exception);
  }
  if (args_->verbose > 0) {
    std::cerr << "\r";
    printInfo(1.0, loss_, std::cerr);
    std::cerr << std::endl;
  }
}

int FastText::getDimension() const {
  return args_->dim;
}

bool FastText::isQuant() const {
  return quant_;
}

bool comparePairs(
    const std::pair<real, std::string>& l,
    const std::pair<real, std::string>& r) {
  return l.first > r.first;
}

=======
>>>>>>> 93501d1069ecb00758137c19bb27a53ee097a0e4
} // namespace fasttext
