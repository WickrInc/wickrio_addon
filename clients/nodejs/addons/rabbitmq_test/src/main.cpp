#include "rabbitmq_iface.h"

using namespace std;

int main (int argc, char** argv) {
	try {
        RabbitMQIface *iface;

        string clientName = "WickrBot1001";

        iface = new RabbitMQIface(clientName);

        if (!iface->init()) {
            std::cout << "Could not initilize the interface!";
        } else {

            string ss = "{ \"action\" : \"get_statistics\" }";

            string response = iface->sendMessage(ss);

            if (response.length() > 0) {
                  cout << response << endl;
            }
        }
    } catch (AMQPException e) {
		std::cout << e.getMessage() << std::endl;
	}

	return 0;

}
