#Installation


##Binary Installation:

###Linux
script installTF.sh will install binary of Tensorflow and other dependencies on Linux.
`bash ./installTF.sh`

###Mac
`sudo easy_install pip`
`sudo pip install --ignore-installed six`
`sudo pip install --ignore-installed numpy`


May want to upgrade pip but might already be upgraded on your machine
`sudo pip install --upgrade pip`

Can find binary for different versions [here](tflearn.org/installation)
The following is for Mac without GPU support
`export TF_BINARY_URL=https://storage.googleapis.com/tensorflow/mac/cpu/tensorflow-1.1.0-py2-none-any.whl`

`sudo pip install $TF_BINARY_URL`

Install other dependencies

`sudo pip install tflearn`
`sudo pip install numpy`
`sudo pip install h5py`
`sudo pip install nltk`



###install Tensorflow with Virtualenv
Virtual environment is helpful when different python projects each with different requirements. Likely not necessary for this project.
Make sure using python 2.7
`python -V`

more detailed instructions on downloadin tensorflow at tensorflow.org

installing some python tools to help installation using virtualenv
`sudo apt-get install python-pip python-dev python-virtualenv`

`virualenv --system-site-packages $(path to directory you want for TF)`
activate Virtualenv with:
`source ($path to directory of project)/bin/activate`
needs to be activated each time you close the terminal


prompt should change to ($name of directory)

update to most recent version of pip
`easy_install -U pip`
install tensorflow to directory
`pip install --upgrade tensorFlow`

Install numpy, h5py and nltk
`pip install numpy`
`pip install h5py`
`pip install nltk`

Install [tflearn](tflearn.org)
`pip install tflearn`

This project also uses punkt and wordnet, but will install handle their installation within the python script. This can be taken out of the chatbot scripts as it only needs to happen once. Can make own python script that just imports

~~~
import nltk
nltk.download('punkt')
from nltk.corpus import wordnet
~~~


##External tools used:
	*tensorflow (apache 2.0)
	*tflearn (mit)
	*numpy (own license at numpy.org/license)
	*h5py (own license at docs.h5py.org)
	*wordnet (own license at wordnet.princeton.edu)
	*nltk (apache 2.0)

nltk is the main reason for using Python. The Stemmer is written there.

#Usage

Create new net classifying intents
>python chatbot.py
Test accuracy of the net using tests defined in tests.json
>python testReply.py

Run net to test questions on command line
>python chatReply.py messenger
or
>python chatReply.py enterprise

Argument sets the context for certain questions. Currently using a filter based on the first argument sent to the program. Include a Context in the intert that specifies to give a reply specific to the question. For more complex tasks it is possible to set an intent in some data structure to specify that the bot is waiting for a certain reply a specific user. There is an older commit in the git history with this implemented.

Intents are written in intents.json and tests are specified in tests.json

Format of intents.json
{"intents":[
	{"tag":
	"patterns":
	"responses":
	}
]
}

Format of tests.json
{"tests":[
	{"tag":
	"question":
	}]
}

#How the net works
loads json and reads each pattern. Splits pattern into tokens (individual words). Gets the synonym of the word. Adds the words and synonyms to a list of words from all the intents and adds the word and tag/synonym and tag to a document that will make training data.

Turns the words into a list of words and for each sentence creates a bag of words, a vector with a 1 when the word is in the sentence and a 1 when it is not. Makes output row with one 1 for where the intent is and the rest 0's. Attaches bag and output row to the training data. train\_x becomes the bag and train\_y the output row.

Creates the net. Currently using .5 dropout with an input and output layer and 3 hidden layers of 64 nodes. Input layer must have as many nodes as there are words. Output must have as many nodes as there are intents. Hidden layers can have however many are desired.

#Building more intents
Building an intent uses "tag", "patterns" and "responses". "tag" just mark what intent it is associated with. Tags do not have to be related to the intent, but it will be easier to remember if they are. 

Patterns build up the net. Need many ways to ask or say the same thing. The net puts some synonyms into the patterns, but should try to include multiple ways of saying.

Responses are word for word the text you want to send the user. Can include multiple ways of saying reply or just one. The net chooses one of the responses to send to the user.


Altering responses in intents.json can be done without needing to retrain the net. You can also separate into two json files (one with tag and patterns and one with tag and responses (+ any other data for formultating the replies e.g. context))


Changes to the tag or the patterns part of intents.json will need to have chatbot.py run for them to be reflected in net



#Changing the net

##Algorithms
Algorithms define the learning patterns
Best algorithms for training neural net are Adam and rmsprop (both SGD with gradient-based optimization). The algorithm can be changed by altering:
`net = tflearn.regression(net)`
expression. The default is Adam, but can specify by changing to :
`net = tflearn.regression(net, optimzer='rmsprop')`

##Stemmer
Stemmer splits the word into The three main stemmer are Porter, Lancaster and Snowball(an improved Porter). NLTK has all three stemmers and can change which one using by changing the line
```
from nltk.stem import SnowballStemmer
stemmer = SnowballStemmer('english')
```

```
from nltk.stem.lancaster import LancasterStemmer
stemmer = LancaserStemmer()
```

```
from nltk.stem.porter import PorterStemmer
stemmer = PorterStemmer()
```

There is also lemmatization matches more words more broadly than stemmer so may be able to remove the wordnet synonyms from the training. 
[article on the difference between stemming and lemmatization](www.ideaeng.com/stemming-lemmatization-0601)
wordNet provides a lemmatizer

~~~
from nltk.stem import WordNetLemmatizer
lemmatizer = WordNetLemmatizer()
lemmatizer.lemmatize("friend")
~~~



##Adding more data
To add the ability to classify more questions, need to alter the json file. Then need to run the training. If the tag or the patterns change then the net needs to be run to reflect those changes.

##Training
Can easily change the parameters of the training. The number of epochs define how many times the net runs through the training data. Larger number of epochs not always successful in improving accuracy vs smaller (eg 10,000 was roughly the same or worse than 3,000)
This may be due to overtraining the net, so it learn to memorize training set and not how to make predictions.

Batch size determine how much of the data the trainer runs through before changing the weights on the nodes.
Smaller batch sizes train the network more quickly. Larger batch take a larger number of epochs to train. Larger batch sizes have made some of the intents unable to classify to the same level of certainty.

Can alter topology of the net quickly by changing number of nodes, number of layers, or activation function (which sets when the node will fire). 
The topologies must match exactly between the chatbot.py, testReply.py and chatReply.py otherwise will fail to import net (except dropout rate which is only in training)

layers can be added or removed by changing the number of lines of:
`net = tflearn.fully\_connected(net, 64)`
In this case the 64 is the number of nodes in that layer of the net. Can be changed to make a larger or smaller net.
Introducing a dropout regularization (randomly disabled a fraction of the neurons and sending the input through that subset of the net) helps the net learn and not memorize. It helps ensure that more nodes get trained and not just ones near the training data. It is especially helpful with small training data and large nets. A dropout rate of 20% is specified with:
`tflearn.dropout(net, .80)`
The second parameter is the likelihood that the node will remain active.

Several places use 20% drop rate as a default, but 50% is also recommended to get the highest variance of the distribution. 50% dropout was able to increase the accuracy of the net greatly and make the test accuracy measure more reflective of the net's ability on new inputs. 20% may perform similarly but may require longer training times to get the same coverage.

Higher dropout decreases the accuracy measure of the training data but should improve the performance of the testing data.

##Testing the net
To be sure that the training has not overtrained, need to perform a measure of the accuracy on new inputs. One way to do this is with validation\_set parameter in the model.fit
`model.fit(train_x, train_y, validation_set=0.1)`

This sets aside a validation set of 10% of the training data.

However this really slows the training, and may not be effective on small amounts of training data as you are not using a portion of it. 
When used for the sample data, there was never above 60% accuracy of validation data even when the testing data was included to increase size of training data.

Another downside is the built-in validation set also does not show if there is a certain intent the net is having difficulty classifying, which you might be able to fix by changing the patterns for that intent.

A way to test the accuracy of the net without using the validation\_set is to create own testing data and run them through the net. This verifies that the net is trained well and is able to adapt to new inputs.

For the testReply.py script there needs to be a json formatted file with tests at the outermost layer. Each test has a tag matching the training data and a question that you want to tested. The tag must match the tag in the intents.json exactly otherwise it will fail to match. An example of the setup is below:
{
"tag":"findFriends",
"question":"How can I find people?"
}

#Ideas for more improvements
Here's a good [guide](https://machinelearningmastery.com/improve-deep-learning-performance) on methods of improving


Creating more training data- nets perform better with larger training data

Adding jitter to words to help identify words that may be misspelt. There are some searches how similar a spelling is. Can either add some of this to the training data or to the answers sent into the net. Probably better with applying to the answer, since we don't want similar spelt words and their synonyms added to the net.
Tools to help with spelling correction:
[Fuzzy Query](https://www.elastic.co/guide/en/elasticsearch/guide/current/fuzzy-query.html) set a similarity to the word and will select words within that measure in Levenshtein  distance (eg one extra letter or swapped letter is distance 1)
[TextBlob](textblob.readthedocs.io) Spelling Correction
[guide to write spelling corrector](norvig.com/spell-correct.html) in python
[MySpell](https://code.google.com/archive/a/apache-extras.org/p/ooo-myspell) in C++


Different topologies of the net perform better on different types of problems. This is something that needs to be experimented. Change topology and test with small sample to see how they perform. Can make wide shallow nets or deeper nets with fewer nodes.

Similar to testing different topologies, can also try different learning methods and polling the results to see which prediction is most common.


This neural net follows the Bag-of-Words style. It's not able to care about the order of the word as that information is stripped out before the sentence sent to the net. It's looking for a number of words that it recognizes and classifying based on how close that is to the patterns it's stored. 

Additionally it is a Feed Forward net as the data travels through a given layer and node once.
[Here is an explanation of several different net types](https://deeplearning4j.org/lstm.html)


Some research has shown convolutional neural nets and recurrent have had better results in classification.

LSTM (long short term memory) could also be a good area of exploration for improving this project. This is a type of recurrent neural net, which loops back through same net for each word and can store some information about the sentence separately. Can help the net decipher the context of the word and change meaning based on that


###Vector Representation

Post of using  vector representations to make nlp bot:
[Guide to making several nlp bots](medium.com/rasa-blog/do-it-yourself-nlp-for-bot-developers-2e2da2817f3d)

I tried implementing the third example with MITIE using the same intents JSON. This was very slow. Probably would have taken 8-10 hours of training.

Some useful tool are Words2Vec and GloVe(global vectors for word representation) both under Apache 2.0 license
They are learning algorithms for word embeddings in multi-dimensional space. They look at where words occur and learn from that.
They will need large training data, but there are open source data sets (the word2vec page lists a number of online training data). The training for this will take longer, but you can import pretrained one and the training may not need to be repeated. This could elinimate the need to use python, as the stemmer no longer needed.


[GloVe](nlp.stanford.edu/projects/glove)
[word2vec](code.google.com/archive/p/word2vec)



