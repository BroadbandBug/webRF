#include <libbladeRF.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <complex>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

bool my_func( int16_t *buffer, const size_t num_samples ){
  char myfifo[] = "/tmp/myfifo";
  struct stat st;
  int fd;
  if (stat(myfifo, &st) != 0)
    mkfifo( myfifo, 0666);
  fd = open(myfifo, O_WRONLY);


  //Let's parse the shit out of this thing.
  //Get the IQ samples and write that shit to an open FIFO
  std::vector<int16_t> real;
  std::vector<int16_t> quad;
  real.reserve(num_samples);
  quad.reserve(num_samples);

  int16_t *dataptr = buffer;

  for( int i = 0;
       i < num_samples;
       i++, dataptr += sizeof(int16_t)*2)
  {
    real.push_back(  *dataptr );
    //cout << "I: " << *dataptr;
    write(fd, dataptr, sizeof(int16_t));
    quad.push_back( *(dataptr + sizeof(int16_t)));
    //cout << " Q: " << *(dataptr + sizeof(int16_t)) << endl;
    write(fd, (dataptr+sizeof(int16_t)), sizeof(int16_t));
  }
  close(fd);
  return false;
}

int main( int argc, char* argv[] ){
    if( argc < 3 ){
      cout << "Usage:\n";
      cout << "fifo_test samp_rate freq" << endl;
      return -1;
    }
    int rate = atoi(argv[1]);
    int freq = atoi(argv[2]);
    int status;
    void *buffer;
    const size_t num_samples = 4096;
    bool done = false;
    struct bladerf_devinfo *devices;

    int nDevices = bladerf_get_device_list( &devices );
    struct bladerf * dev;
    bladerf_open_with_devinfo( &dev, &devices[0] );

    struct bladerf_rational_rate desired;
    //desired.integer = 20e6;
    desired.integer = rate;
    desired.den     = 1;
    desired.num     = 1;
    struct bladerf_rational_rate out_samp;

    unsigned int actualbw;
    bladerf_set_lna_gain( dev, BLADERF_LNA_GAIN_MID );
    bladerf_set_rational_sample_rate( dev, BLADERF_MODULE_RX, &desired, &out_samp);
    bladerf_set_frequency( dev, BLADERF_MODULE_RX, freq);
    /*if( bladerf_set_bandwidth( dev, BLADERF_MODULE_RX, 1500000, &actualbw) ){
      cout << "Error: Couldn't set desired BW" << endl;
      return true;
    }
    cout << "Actual BW: " << actualbw << endl;
    */

    // Allocate a sample buffer.
    // Note that 4096 samples = 4096 int16_t IQ pairs = 2 * 4096 int16_t values
    buffer = malloc(num_samples * 2 * sizeof(int16_t));
    if (buffer == NULL) {
        perror("malloc");
        return BLADERF_ERR_MEM;
    }
    // Configure the device's RX module for use with the sync interface
    status = bladerf_sync_config(dev, BLADERF_MODULE_RX, BLADERF_FORMAT_SC16_Q11,
                                64, 16384, 16, 3500);
    if (status != 0) {
        fprintf(stderr, "Failed to configure sync interface: %s\n",
                bladerf_strerror(status));
        return status;
    }
    // We must always enable the RX module before attempting to RX samples
    status = bladerf_enable_module(dev, BLADERF_MODULE_RX, true);
    if (status != 0) {
        fprintf(stderr, "Failed to enable RX module: %s\n",
                bladerf_strerror(status));
        return status;
    }
    // Receive samples and do work on them.
    while (status == 0 && !done) {
        status = bladerf_sync_rx(dev, buffer, num_samples, NULL, 3500);
        if (status == 0) {
            //cout << "Received " << *buffer << endl;
            //done = do_work(buffer, num_samples);
            done = my_func((int16_t*) buffer, num_samples);
        } else {
            fprintf(stderr, "Failed to RX samples: %s\n",
                    bladerf_strerror(status));
        }
    }
    // Disable RX module, shutting down our underlying RX stream
    status = bladerf_enable_module(dev, BLADERF_MODULE_RX, false);
    if (status != 0) {
        fprintf(stderr, "Failed to disable RX module: %s\n",
                bladerf_strerror(status));
    }
    // Free up our resources
    free(buffer);
  return false;
}


