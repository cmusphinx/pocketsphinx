import os
CUDA_VISIBLE_DEVICES = '0'
os.environ["CUDA_VISIBLE_DEVICES"] = CUDA_VISIBLE_DEVICES
from keras.models import load_model
import numpy as np
import socket
import struct
import time
def predictFrame(model,frame,weight=1,offset=0):
	scores = model.predict(frame)
	
	n_active = scores.shape[1]
	# print freqs
	# scores /= freqs + (1.0 / len(freqs))
	scores = np.log(scores)/np.log(1.0001)
	scores *= -1
	scores -= np.min(scores,axis=1).reshape(-1,1)
	# scores = scores.astype(int)
	scores *= weight
	scores += offset
	truncateToShort = lambda x: 32676 if x > 32767 else (-32768 if x < -32768 else x)
	vf = np.vectorize(truncateToShort)
	scores = vf(scores)
	print scores
	r_str = struct.pack('%sh' % len(scores[0]), *scores[0])

	# scores /= np.sum(scores,axis=0)
	return r_str

if __name__ == '__main__':
	model_name = "/home/mshah1/GSOC/bestModels/best_CI.h5"
	HOST, PORT = '', 9000
	listen_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	listen_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	listen_socket.bind((HOST, PORT))
	listen_socket.listen(1)
	client_connection = None
	print 'Serving HTTP on port %s ...' % PORT
	while True:
		if client_connection == None:
		    client_connection, client_address = listen_socket.accept()
		    model = load_model(model_name)
		r = client_connection.recv(4)
		if 	len(r) != 4:
			print "Expected 4 bytes for PACKET_LEN got " + str(len(r))
			continue	
		packet_len = struct.unpack('i',r)[0]
		print packet_len
		full_req = ""
		while len(full_req) < packet_len:
			partial_req = client_connection.recv(packet_len)
			full_req += partial_req
		assert(len(full_req) == packet_len)
		
		frame = full_req
		frame = list(struct.unpack('%sf' % 225, frame))
		frame = np.array([frame])
		
		resp = str(predictFrame(model,frame,weight=0.00296036))
		print time.time()
		client_connection.send(resp)
		print time.time()