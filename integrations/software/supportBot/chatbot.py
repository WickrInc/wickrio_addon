import nltk
nltk.download('punkt')
nltk.download('wordnet')
from nltk.stem import SnowballStemmer
stemmer = SnowballStemmer('english')
from nltk.corpus import wordnet
import nltk.data
import numpy as np
import tflearn
import tensorflow as tf
import random
import json
import cPickle


with open('intents.json') as json_data:
	intents = json.load(json_data)


words = []
classes = []
documents = []

#words that will not be included in the training data as provide no real meaning
ignore_words =['?', 'is','are', 'the', 'a', 'an']


#reading json file
for intent in intents['intents']:
	for pattern in intent['patterns']:
		#split sentence into words
		w = nltk.word_tokenize(pattern)
		synonyms = []
		#add synonyms found on wordnet to list of words associated with this intent
		#increase size of training data but unclear if improves accuracy of model
		for word in w:
			for syn in wordnet.synsets(word):
				for lemma in syn.lemmas():
					synonyms.append(lemma.name())
		#put all words and synonyms into a list and creates a second list of the word and intent
		words.extend(synonyms)
		documents.append((synonyms, intent['tag']))
		words.extend(w)
		documents.append((w, intent['tag']))
		#adds the tag to list of tags
		if intent['tag'] not in classes:
			classes.append(intent['tag'])


#stems word to root to find related words
words = [stemmer.stem(w.lower()) for w in words if w not in ignore_words]

words = sorted(list(set(words)))
classes = sorted(list(set(classes)))

training = []
output = []
output_empty =[0]*len(classes)

#Converts list of words to array with 0 if word not there and 1 if it is
for doc in documents:
	bag = []
	pattern_words = doc[0]
	pattern_words = [stemmer.stem(word.lower()) for word in pattern_words]
	for w in words:
		bag.append(1) if w in pattern_words else bag.append(0)
	#creates an row matching the intent
	output_row = list(output_empty)
	output_row[classes.index(doc[1])] = 1
	training.append([bag, output_row])

random.shuffle(training)
#training has tuples of bag of words matched with the output row
training = np.array(training)


#train_x is the bag of words, the pattern input into the net
train_x = list(training[:,0])
#train_y is the output row of which specifies which tag
train_y = list(training[:,1])

tf.reset_default_graph()
#create neural net (except for the dropout it needs to match net of anything using it)
#Can add more layers by adding another net = tflearn.fully_connected(net, (# of nodes))
net = tflearn.input_data(shape=[None, len(train_x[0])])
#Introduce dropout for nodes in the net
net = tflearn.dropout(net, .50)
#Define shape of the net and activation function for each layer(default is linear)
net = tflearn.fully_connected(net, 64)
net = tflearn.fully_connected(net, 64)
net = tflearn.fully_connected(net, 64)
net = tflearn.fully_connected(net, len(train_y[0]), activation ='softmax')
#Set Optimizer, which defines training rate (default is Adam)
net = tflearn.regression(net)

model = tflearn.DNN(net, tensorboard_dir='tflearn_logs')
#training- change number of steps with n_epoch
#model.fit(train_x, train_y, validation_set=(X_val, Y_val))
model.fit(train_x, train_y, n_epoch=1000, batch_size = 32, show_metric =True)
#save model of neural net so you can use it in other programs
model.save('model.tflearn')
cPickle.dump({'words':words, 'classes':classes, 'train_x':train_x, 'train_y':train_y}, open("training_data", "wb"))
