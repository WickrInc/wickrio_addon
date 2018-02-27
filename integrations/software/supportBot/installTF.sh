#Installation script for chatbot


apt-get install python-pip python-dev build-essential

pip install --upgrade pip

export TF_BINARY_URL=https://storage.googleapis.com/tensorflow/linux/cpu/tensorflow-1.1.0-cp27-none-linux_x86_64.whl

pip install $TF_BINARY_URL

pip install tflearn

pip install numpy
pip install h5py
pip install nltk


