import nltk
from nltk.stem import SnowballStemmer
stemmer = SnowballStemmer('english')

import numpy as np
import tflearn
import tensorflow as tf
import random
import cPickle
import json

#First argument should be the type of account (eg messenger, enterprise)

data = cPickle.load(open("training_data", "rb"))
words = data['words']
classes = data['classes']
train_x = data['train_x']
train_y = data['train_y']

#opens json with right replies
with open('intents.json') as json_data:
	intents = json.load(json_data)

#opens json with test cases
with open('tests.json') as json_data:
	tests = json.load(json_data)


#create net. Must match the setup in chatbot.py. Each fully_connected makes new
#layer in net of 64 nodes
net = tflearn.input_data(shape=[None, len(train_x[0])])
net = tflearn.fully_connected(net, 64)
net = tflearn.fully_connected(net, 64)
net = tflearn.fully_connected(net, 64)
net = tflearn.fully_connected(net,len(train_y[0]), activation ='softmax')
net = tflearn.regression(net)
model = tflearn.DNN(net, tensorboard_dir ='tflearn_logs')

def cleanUp(sentence):
	words = nltk.word_tokenize(sentence)
	words =[stemmer.stem(word.lower()) for word in words]
	return words

#changes sentence into format accepted by the neural net
def bow(sentence, words):
 	sentence_words = cleanUp(sentence)
	bag = [0]*len(words)
	for word in sentence_words:
		for i, w in enumerate(words):
			if w == word:
				bag[i] = 1
	return(np.array(bag))

model.load('./model.tflearn')

ERROR_THRESHOLD =.20
def classify(sentence):
	results = model.predict([bow(sentence, words)])[0]
	results = [[i,r] for i,r in enumerate(results) if r > ERROR_THRESHOLD]
	results.sort(key=lambda x:x[1], reverse = True)
	return_list = []
	for r in results:
		return_list.append((classes[r[0]],r[1]))
	return return_list

def testPrediction(sentence, category):
	ans = classify(sentence)
	if not ans:
		print "Unable to classify:"
		print  sentence, "should be", category
		print
		return False
	if ans[0][0] == category:
		print "Correctly classified as", ans[0][0], "certainty",ans[0][1]
		return True
	else:
		print "Incorrect classification:"
		print sentence, "is classified as", ans[0][0], "with certainty", ans[0][1],"but should be", category
		print
		return False

count = 0
countTrue = 0
print
print "Incorrect classifications:"
for attempt in tests["tests"]:
	count = count + 1 
	if testPrediction(attempt["question"], attempt["tag"]):
		countTrue = countTrue + 1
print
print "The number of correct classifications is ", countTrue, " out of ", count
print "Percentage correct: ", round(float(countTrue * 100)/float(count), 2), "%"

