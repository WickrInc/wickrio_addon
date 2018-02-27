import sys
import nltk
from nltk.stem import SnowballStemmer
stemmer = SnowballStemmer('english')

import numpy as np
import tflearn
import tensorflow as tf
import random
import cPickle
import json

#First argument can be messenger or enterprise to filter responses to those application if applicable


#Create net framework (needs to match latest chatbot net in topology and training type to load correctly)
data = cPickle.load(open("training_data", "rb"))
words = data['words']
classes = data['classes']
train_x = data['train_x']
train_y = data['train_y']
net = tflearn.input_data(shape=[None, len(train_x[0])])
net = tflearn.fully_connected(net, 64)
net = tflearn.fully_connected(net, 64)
net = tflearn.fully_connected(net, 64)
net = tflearn.fully_connected(net,len(train_y[0]), activation ='softmax')
net = tflearn.regression(net)
model = tflearn.DNN(net, tensorboard_dir ='tflearn_logs')
model.load('./model.tflearn')


with open('intents.json') as json_data:
	intents = json.load(json_data)

def cleanUp(sentence):
	words = nltk.word_tokenize(sentence)
	words =[stemmer.stem(word.lower()) for word in words]
	return words

#create bag of words for the sentence
def bow(sentence, words):
 	sentence_words = cleanUp(sentence)
	bag = [0]*len(words)
	for word in sentence_words:
		for i, w in enumerate(words):
			if w == word:
				bag[i] = 1
	return(np.array(bag))

context = {}

#ERROR_THRESHOLD lets you control the minimum error you consider for an answer
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
	if classify(sentence)[0][0] == category:
		return True
	else:
		return False

def response(sentence, userID):
	if userID not in context:
		context[userID]= []
	results = classify(sentence)
	if results:
		while results:
			for i in intents['intents']:
				if i['tag'] == results[0][0]:
					if 'context' in i:
						return resolveContext(sentence, userID, i)
					else:
						return random.choice(i['responses'])
			results.pop(0)
	return "Sorry this question was not understood. Maybe try rephrasing it or contact our representative"


#Figures out what to do with the context filters 'application' and 'device' which specify
#that the answer varies based on what device the user plans on using
def resolveContext(sentence, userID, intent):
	if intent['context'] == 'application':
		if len(sys.argv) > 1:
			if sys.argv[1] == 'messenger':
				return random.choice(intent['messenger'])
			elif sys.agrv[1] == 'enterprise':
				return random.choice(intent['enterprise'])
		else:
			return random.choice(intent['responses'])
	#context for device not used currently
	elif intent['context'] == 'device':
		return random.choice(intent['responses'])

print
print
text = raw_input("Enter a question: ")
while text != "quit":
	print(response(text, 123))
	text = raw_input("Do you have any more questions? ")

