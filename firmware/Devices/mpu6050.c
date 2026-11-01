#include "mpu6050.h"
#include <string.h>

/* The instrument consumes a 50 Hz stream at +/-2 g and +/-250 dps. Read-back
 * validation prevents accepting a baseline with an unexpected sensor scale. */
static const uint8_t acquisition[4]={19,3,0,0}; /* 1 kHz/(19+1), DLPF3, gyroFS0, accelFS0 */
BusResult mpu6050_init(Mpu6050 *d) {
    uint8_t identity,wake=1,actual[4];
    BusResult result=d->bus.read(d->bus.context,d->address,0x75,&identity,1);
    if(result!=BUS_OK)return result;
    if(identity!=0x68)return BUS_ERROR;
    result=d->bus.write(d->bus.context,d->address,0x6b,&wake,1);
    if(result!=BUS_OK)return result;
    result=d->bus.write(d->bus.context,d->address,0x19,acquisition,sizeof acquisition);
    if(result!=BUS_OK)return result;
    result=d->bus.read(d->bus.context,d->address,0x19,actual,sizeof actual);
    if(result!=BUS_OK)return result;
    if(memcmp(actual,acquisition,sizeof actual))return BUS_ERROR;
    result=d->bus.read(d->bus.context,d->address,0x6b,&wake,1);
    if(result!=BUS_OK)return result;
    return (wake&0x7f)==1?BUS_OK:BUS_ERROR;
}
static int32_t be16(const uint8_t *bytes) {
    unsigned u=(unsigned)bytes[0]*256+bytes[1];
    return u&0x8000u?(int32_t)u-65536:(int32_t)u;
}
void mpu6050_decode(const uint8_t frame[14],ImuSample *sample) {
    /* Temperature separates the two vectors in the wire format. */
    const unsigned accelerometer_offsets[3]={0,2,4},gyroscope_offsets[3]={8,10,12};
    for(unsigned channel=0;channel<3;++channel) {
        sample->accel_mg[channel]=be16(frame+accelerometer_offsets[channel])*1000/16384;
        sample->gyro_mdps[channel]=be16(frame+gyroscope_offsets[channel])*1000/131;
    }
    sample->temperature_mc=36530+be16(frame+6)*1000/340;
}
BusResult mpu6050_read(Mpu6050 *d,ImuSample *sample) {
    uint8_t frame[14];
    BusResult result=d->bus.read(d->bus.context,d->address,0x3b,frame,sizeof frame);
    if(result==BUS_OK)mpu6050_decode(frame,sample);
    return result;
}
