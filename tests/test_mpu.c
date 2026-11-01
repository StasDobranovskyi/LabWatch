#include "mpu6050.h"
#include "fixtures/mpu-captured.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
typedef struct { uint8_t map[128];unsigned call,fail_at;BusResult failure;bool bad_readback; } SensorModel;
static BusResult transfer(SensorModel *m,uint8_t address,uint8_t reg,void *p,size_t n,bool write) {
    assert(address==0x68&&reg+n<=128);
    if(++m->call==m->fail_at)return m->failure;
    if(write)memcpy(m->map+reg,p,n);
    else {memcpy(p,m->map+reg,n);if(m->bad_readback&&reg==0x19)((uint8_t *)p)[0]^=1;}
    return BUS_OK;
}
static BusResult rd(void *c,uint8_t a,uint8_t r,uint8_t *p,size_t n) { return transfer(c,a,r,p,n,false); }
static BusResult wr(void *c,uint8_t a,uint8_t r,const uint8_t *p,size_t n) { return transfer(c,a,r,(void *)p,n,true); }
int main(void) {
    unsigned failures=0;
    for(unsigned call=1;call<=5;++call)for(BusResult e=BUS_NACK;e<=BUS_ERROR;++e) {
        SensorModel m={.fail_at=call,.failure=e};m.map[0x75]=0x68;Mpu6050 d={{&m,rd,wr},0x68};
        assert(mpu6050_init(&d)==e&&m.call==call);++failures;
    }
    SensorModel m={0};m.map[0x75]=0x68;Mpu6050 d={{&m,rd,wr},0x68};
    assert(mpu6050_init(&d)==BUS_OK&&m.call==5);
    assert(m.map[0x19]==19&&m.map[0x1a]==3&&m.map[0x1b]==0&&m.map[0x1c]==0&&m.map[0x6b]==1);
    m.bad_readback=true;assert(mpu6050_init(&d)==BUS_ERROR);m.bad_readback=false;
    m.map[0x75]=0;assert(mpu6050_init(&d)==BUS_ERROR);m.map[0x75]=0x68;
    ImuSample sample;
    memcpy(m.map+0x3b,captured_stationary,14);assert(mpu6050_read(&d,&sample)==BUS_OK&&sample.accel_mg[2]==1000&&sample.temperature_mc==24001);
    memcpy(m.map+0x3b,captured_moving,14);assert(mpu6050_read(&d,&sample)==BUS_OK&&sample.accel_mg[0]==1999&&sample.gyro_mdps[0]==CAPTURED_GYRO_X_MDPS);
    for(BusResult e=BUS_NACK;e<=BUS_ERROR;++e) {
        ImuSample before=sample;m.fail_at=m.call+1;m.failure=e;
        assert(mpu6050_read(&d,&sample)==e&&!memcmp(&sample,&before,sizeof sample));
    }
    /* Exhaust all signed raw values on every axis; expected conversion uses an
     * independent signed integer, not the decoder's byte arithmetic. */
    for(int raw=-32768;raw<=32767;++raw)for(unsigned axis=0;axis<3;++axis) {
        uint8_t frame[14]={0};unsigned u=(unsigned)raw&65535;
        frame[2*axis]=(uint8_t)(u/256);frame[2*axis+1]=(uint8_t)u;
        frame[8+2*axis]=(uint8_t)(u/256);frame[9+2*axis]=(uint8_t)u;
        mpu6050_decode(frame,&sample);
        assert(sample.accel_mg[axis]==raw*1000/16384&&sample.gyro_mdps[axis]==raw*1000/131);
    }
    printf("MPU host: 50Hz configuration read-back, %u init failures, preserved sample on read failure, own Lab2 vectors and 196608 signed-axis cases passed\n",failures);
    return 0;
}
