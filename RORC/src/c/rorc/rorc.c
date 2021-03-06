#include "rorc.h"
#include <pda.h>
#include "rorc_macros.h"
#include "ddl_def.h"
#include "aux.h"

#define MAX_STAT 4096
#define MAX_WAIT 1000000

void rorcReset (uintptr_t bar, int option, int pci_loop_per_usec){
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  int prorc_cmd;
  long long int longret, timeout;

  timeout = DDL_RESPONSE_TIME * pci_loop_per_usec;
  prorc_cmd = 0;
  if (option & RORC_RESET_DIU)
    prorc_cmd |= DRORC_CMD_RESET_DIU;
  if (option & RORC_RESET_FF)
    prorc_cmd |= DRORC_CMD_CLEAR_RXFF | DRORC_CMD_CLEAR_TXFF;
  if (option & RORC_RESET_FIFOS)
    prorc_cmd |= DRORC_CMD_CLEAR_FIFOS;
  if (option & RORC_RESET_ERROR)
    prorc_cmd |= DRORC_CMD_CLEAR_ERROR;
  if (option & RORC_RESET_COUNTERS)
    prorc_cmd |= DRORC_CMD_CLEAR_COUNTERS;
  if (prorc_cmd)   // any reset
    {
      rorcWriteReg(bar, C_CSR, (__u32) prorc_cmd);
    }
    if (option & RORC_RESET_SIU)
    {
      rorcPutCommandRegister(bar, PRORC_CMD_RESET_SIU);
      longret = ddlWaitStatus(bar, timeout);
      if (longret < timeout)
        ddlReadStatus(bar);
    }
    if (!option || (option & RORC_RESET_RORC))
    {
      rorcWriteReg(bar, RCSR, DRORC_CMD_RESET_CHAN);  //channel reset
    }
  }

int rorcEmptyDataFifos(uintptr_t bar, int empty_time)

/* try to empty D-RORC's data FIFOs                            
               empty_time:  time-out value in usecs
 * Returns:    RORC_STATUS_OK or RORC_TIMEOUT 
 */

{
  struct timeval start_tv, now_tv;
  int dsec, dusec;
  double dtime;
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  gettimeofday(&start_tv, NULL);
  dtime = 0;
  while (dtime < empty_time){
    if (!rorcCheckRxData(bar))
      return (RORC_STATUS_OK);

    rorcWriteReg(bar, C_CSR, (__u32) DRORC_CMD_CLEAR_FIFOS);
    gettimeofday(&now_tv, NULL);
    elapsed(&now_tv, &start_tv, &dsec, &dusec);
    dtime = (double)dsec * 1000000 + (double)dusec;
  }

  if (rorcCheckRxData(bar))        // last check
    return (RORC_TIMEOUT);
  else
    return (RORC_STATUS_OK);
}

int rorcArmDDL(uintptr_t bar, int option, int diu_version, int pci_loop_per_usec){
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  int print = 0; // -1;
  int stop = 1;
  long long int TimeOut;

  TimeOut = DDL_RESPONSE_TIME * pci_loop_per_usec;

  if (diu_version){
    if (option & RORC_RESET_FEE){
      /* not implemented */
      return RORC_CMD_NOT_ALLOWED;
    }
    if (option & RORC_RESET_SIU){
      long ret = ddlResetSiu(bar, 0, 3, TimeOut, diu_version, pci_loop_per_usec);
      if (ret == -1){
        printf(" Unsuccessful SIU reset\n");
        return RORC_NOT_ACCEPTED;
      }
    }
    if (option & RORC_LINK_UP){
      if (diu_version <= NEW){
        long ret = ddlLinkUp(bar, 1, print, stop, TimeOut, diu_version, pci_loop_per_usec);
        if (ret == -1){
          printf(" Can not read DIU status");
          return RORC_LINK_NOT_ON;
        }
      }
      else{
        rorcReset(bar, RORC_RESET_RORC, pci_loop_per_usec);
        rorcReset(bar, RORC_RESET_DIU, pci_loop_per_usec);
        rorcReset(bar, RORC_RESET_SIU, pci_loop_per_usec);
        usleep(100000); 

        if (rorcCheckLink(bar))
          return (RORC_LINK_NOT_ON);
        if (rorcEmptyDataFifos(bar, 100000))
          return (RORC_TIMEOUT);

        rorcReset(bar, RORC_RESET_SIU, pci_loop_per_usec);
        rorcReset(bar, RORC_RESET_DIU, pci_loop_per_usec);
        rorcReset(bar, RORC_RESET_RORC, pci_loop_per_usec);
        usleep(100000);

        if (rorcCheckLink(bar))
          return (RORC_LINK_NOT_ON);
      }
    }
    if (option & RORC_RESET_DIU)
       rorcReset(bar, RORC_RESET_DIU, pci_loop_per_usec);
  }
  else{
    printf(" No DIU plugged into the RORC\n");
    return (RORC_LINK_NOT_ON);
  }
  if (option & RORC_RESET_FF)
    rorcReset(bar, RORC_RESET_FF, pci_loop_per_usec);
  if (option & RORC_RESET_RORC)
    rorcReset(bar, RORC_RESET_RORC, pci_loop_per_usec);
  
  return RORC_STATUS_OK;
}
 
int rorcCheckRxFreeFifo(uintptr_t bar)  //RX Address FIFO
{
   __u32 st;
DEBUG_PRINTF(PDADEBUG_ENTER, "");
  st = rorcReadReg(bar, C_CSR);
  if (st & DRORC_STAT_RXAFF_FULL)  
    return (RORC_FF_FULL);
  else if (st & DRORC_STAT_RXAFF_EMPTY)
    return (RORC_FF_EMPTY);
  else
    return (RORC_STATUS_OK);  //Not Empty
  
}

__u32 rorcReadFw(uintptr_t bar){
  __u32 fw;

  fw = rorcReadReg (bar, RFID);

  return (fw);
}

void rorcInterpretVersion(__u32 x){
  int major, minor, month, day, year;

  char* monthName[] = {"unknown month", "January", "February", "March",
                       "April", "May", "June", "July", "August",
                       "September", "October", "November", "December",
                       "unknown month", "unknown month", "unknown month"};

  major   = rorcFWVersMajor(x);
  minor   = rorcFWVersMinor(x);
  year    = (x >>  9) & 0xf;
  month   = (x >>  5) & 0xf;
  day     =  x        & 0x1f;

  printf(" Version: %d.%d\n Release date : %s %d 20%02d\n",
           major, minor, monthName[month], day, year);

}

int rorcStartDataReceiver(uintptr_t bar,
                          unsigned long   readyFifoBaseAddress, int rorc_revision){
  int fw_major, fw_minor;
  unsigned long fw;
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  rorcWriteReg(bar, C_RRBAR, (readyFifoBaseAddress & 0xffffffff));
  if (rorc_revision >= RORC_REVISION_DRORC2){
    fw = rorcReadFw(bar);
    fw_major = rorcFWVersMajor(fw);
    fw_minor = rorcFWVersMinor(fw);
    if ((rorc_revision >= RORC_REVISION_PCIEXP) ||
        (fw_major > 2) || ((fw_major == 2) && (fw_minor >= 16))){
#if __WORDSIZE > 32
      rorcWriteReg(bar, C_RRBX, (readyFifoBaseAddress >> 32));
#else
      rorcWriteReg(bar, C_RRBX, 0x0);
#endif
    }  
  }
  if (!(rorcReadReg(bar, C_CSR) & DRORC_CMD_DATA_RX_ON_OFF))
    rorcWriteReg(bar, C_CSR, DRORC_CMD_DATA_RX_ON_OFF);

  return (RORC_STATUS_OK);
}

int rorcStopDataReceiver(uintptr_t bar){
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  if (rorcReadReg(bar, C_CSR) & DRORC_CMD_DATA_RX_ON_OFF)
    rorcWriteReg(bar, C_CSR, DRORC_CMD_DATA_RX_ON_OFF);

  return (RORC_STATUS_OK);
}

int rorcStartTrigger(uintptr_t bar, long long int timeout, stword_t *stw){
  int ret;
  long long int longret;
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  ret = ddlSendCommand(bar, DDL_DEST_FEE, RDYRX, 0, 0, timeout);
  if (ret == RORC_LINK_NOT_ON)
    return (ret);
  if (ret == RORC_TIMEOUT)
    return RORC_STATUS_ERROR;

  longret = ddlWaitStatus(bar, timeout);
  if (longret >= timeout)
    return RORC_NOT_ACCEPTED;
  else
    *stw = ddlReadStatus(bar);

  return RORC_STATUS_OK;
}

int rorcStopTrigger(uintptr_t bar, long long int timeout, stword_t *stw)
{
  int ret;
  long long int longret;
  int once = 0;

 repeat:
  ret = ddlSendCommand(bar, DDL_DEST_FEE, EOBTR, 0, 0, timeout);
  if (ret == RORC_LINK_NOT_ON)
    return (ret);
  if (ret == RORC_TIMEOUT)
    return RORC_STATUS_ERROR;

  longret = ddlWaitStatus(bar, timeout);
  if (longret >= timeout)
  {
    if (once == 0)
    {
      once = 1;
      goto repeat;
    }
    return RORC_NOT_ACCEPTED;
  }
  else
    *stw = ddlReadStatus(bar);

  return RORC_STATUS_OK;
}

int rorcArmDataGenerator(uintptr_t bar,
                         __u32           initEventNumber,
                         __u32           initDataWord,
                         int             dataPattern,
                         int             eventLen,
                         int             seed,
                         int             *rounded_len){
  unsigned long blockLen;

  if ((eventLen < 1) || (eventLen >= 0x00080000))
    return (RORC_INVALID_PARAM);

  *rounded_len = eventLen;

  if (seed){
    /* round to the nearest lower power of two */
    *rounded_len = roundPowerOf2(eventLen);
    blockLen = ((*rounded_len - 1) << 4) | dataPattern;
    blockLen |= 0x80000000;
    rorcWriteReg(bar, C_DG2, seed);
  }
  else{
    blockLen = ((eventLen - 1) << 4) | dataPattern;
    rorcWriteReg(bar, C_DG2, initDataWord);
  }

  rorcWriteReg(bar, C_DG1, blockLen);
  rorcWriteReg(bar, C_DG3, initEventNumber);

  return (RORC_STATUS_OK);
}

int rorcParamOn(uintptr_t bar, int param){
  if (param != PRORC_PARAM_LOOPB)
    return (RORC_INVALID_PARAM);
  if (!rorcCheckLoopBack(bar))
    rorcChangeLoopBack(bar);

  return (RORC_STATUS_OK);
}

int rorcParamOff(uintptr_t bar)
// switches off both loopback and stop_on_error
{
  if (rorcCheckLoopBack(bar))
    rorcChangeLoopBack(bar);
  
  return (RORC_STATUS_OK);
}

int rorcStartDataGenerator(uintptr_t bar, __u32 maxLoop){
  __u32 cycle;

  if (maxLoop) {
    cycle = (maxLoop - 1) & 0x7fffffff;
  } else {
    cycle = 0x80000000;
  }

  rorcWriteReg(bar, C_DG4, cycle);
  rorcWriteReg(bar, C_CSR, DRORC_CMD_START_DG);

  return (RORC_STATUS_OK);
}

int rorcStopDataGenerator(uint32_t *bar)
{
  rorcWriteReg(bar, C_CSR, DRORC_CMD_STOP_DG);
  
  return (RORC_STATUS_OK);
}

void rorcBuildHwSerial(char data[], unsigned int rorcRevisionNumber, int versionMajor,
                       int versionMinor, char cPld[], int numberOfChannels,
                       int serialNumber)
{
  int i;
  int versionPosition, channelPosition, ldPosition, serialNumberPosition;
  // For the CRORC the ID positions are:
  versionPosition = 7;
  channelPosition = 11;
  ldPosition = 20;
  serialNumberPosition = 33;

  data[0] = '\0';
  if (rorcRevisionNumber != RORC_REVISION_CRORC){
    printf("Card is not a CRORC (rorcBuildHwSerial is implemented only for CRORC)\n");
    return;
  }

  // Filling data with spaces.
  for (i = 0; i < DDL_MAX_HW_ID-1; i++)
    data[i] = ' ';
  
  // Adding hw ID text.
  sprintf(data, "%s", "CRORC1");
  data[strlen("CRORC1")] = ' ';
  
  // Adding hw version; version position is 7 for the CRORC.
  sprintf(&data[versionPosition], "%1dv%1d", versionMajor, versionMinor);
  data[versionPosition + 3] = ' ';
  
  /* add device type */
  if (ldPosition){
    sprintf(&data[ldPosition - 4],"LD: %s", cPld);
    data[ldPosition + strlen(cPld)] = ' ';
  }
  
  // Adding number of channels */
  if (channelPosition){
    if (numberOfChannels == 1)
      sprintf(&data[channelPosition], "%s", "PLG.");
    else if (numberOfChannels == 2)
      sprintf(&data[channelPosition], "%s", "INT.");
    else
      sprintf(&data[channelPosition], "%s%02d", "Ch", numberOfChannels);
    data[channelPosition + 4] = ' ';
  }
  
  /* add serial number */
  sprintf(&data[serialNumberPosition - 5],"S/N: %05d", serialNumber);
  data[serialNumberPosition + 5] = ' ';

  data[DDL_MAX_HW_ID-1] = '\0';
  trim(data);
}


const char* rorcSerial(uint32_t* bar, int rorc_revision){
//  unsigned txtlen;
//  __u8 address;
  static char data[DDL_MAX_HW_ID];
//  char txt[20], checktxt[20];
//  int versionPosition, channelPosition, ldPosition, serialNumberPosition;
//  // For the CRORC the ID positions are:
//  versionPosition = 7;
//  channelPosition = 11;
//  ldPosition = 20;
//  serialNumberPosition = 33;
  
  if (rorc_revision != RORC_REVISION_CRORC){
    printf("Card is not a CRORC (rorcSerial is only implemented for CRORC)\n");
    return NULL;
  }
  else // CRORC
  {
    // Read FLASH memory
    unsigned int flashAddress  = FLASH_SN_ADDRESS;
    initFlash(bar, flashAddress, 10); // TODO check return code

    data[DDL_MAX_HW_ID - 1] = '\0';
    for (int i = 0; i < DDL_MAX_HW_ID - 1; i+=2, flashAddress++)
    {
      readFlashWord(bar, flashAddress, &data[i], 10); // TODO check return code
      if ((data[i] == '\0') || (data[i+1] == '\0'))
        break;
    }
  }

  return data;
}

void setLoopPerSec(long long int *loop_per_usec, double *pci_loop_per_usec, uintptr_t bar){
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  struct timeval tv1, tv2;
  int dsec, dusec;
  double dtime, max_loop;
  int i;

  max_loop = 1000000;
  gettimeofday(&tv1, NULL);

  for (i = 0; i < max_loop; i++){};

  gettimeofday(&tv2, NULL);
  elapsed(&tv2, &tv1, &dsec, &dusec);
  dtime = (double)dsec * 1000000 + (double)dusec;
  *loop_per_usec = (double)max_loop/dtime;
  if (*loop_per_usec < 1)
    *loop_per_usec = 1;
  // printf("memory loop_per_usec: %lld\n", prorc_dev->loop_per_usec);

  /* calibrate PCI loop time for time-outs */
  max_loop = 1000;
  gettimeofday(&tv1, NULL);

  for (i = 0; i < max_loop; i++) {
    (void) rorcCheckRxStatus(bar); // XXX Cast to void to explicitly discard returned value
  }

  gettimeofday(&tv2, NULL);
  elapsed(&tv2, &tv1, &dsec, &dusec);
  dtime = (double)dsec * 1000000 + (double)dusec;
  *pci_loop_per_usec = (double)max_loop/dtime;
}

unsigned initFlash(uintptr_t bar, unsigned address, int sleept)
{
  unsigned stat;

  // Clear Status register
  rorcWriteReg(bar, F_IFDSR, 0x03000050);
  usleep(10*sleept);

  // Set ASYNCH mode (Configuration Register 0xBDDF)
  rorcWriteReg(bar, F_IFDSR, 0x0100bddf);
  usleep(sleept);
  rorcWriteReg(bar, F_IFDSR, 0x03000060);
  usleep(sleept);
  rorcWriteReg(bar, F_IFDSR, 0x03000003);
  usleep(sleept);

  // Read Status register
  rorcWriteReg(bar, F_IFDSR, address);
  usleep(sleept);
  rorcWriteReg(bar, F_IFDSR, 0x03000070);
  usleep(sleept);
  stat = readFlashStatus(bar, 1);

  return (stat);
}

unsigned readFlashStatus(uintptr_t bar, int sleept)
{
  unsigned stat;

  rorcWriteReg(bar, F_IFDSR, 0x04000000);
  usleep(sleept);
  stat = rorcReadReg(bar, F_IADR);

  return (stat);
}

int checkFlashStatus(uintptr_t bar, int timeout)
{
  unsigned stat;
  int i = 0;

  while (1)
  {
    stat = readFlashStatus(bar, 1);
    if (stat == 0x80)
      break;
    if (timeout && (++i >= timeout))
      return (RORC_TIMEOUT);
    usleep(100);
  }

  return (RORC_STATUS_OK);

}

int unlockFlashBlock(uintptr_t bar, unsigned address, int sleept)
{
  int ret;

  rorcWriteReg(bar, F_IFDSR, address);
  usleep(sleept);
  rorcWriteReg(bar, F_IFDSR, 0x03000060);
  usleep(sleept);
  rorcWriteReg(bar, F_IFDSR, 0x030000d0);
  usleep(sleept);
  ret = checkFlashStatus(bar, MAX_WAIT);

  return (ret);
}

int eraseFlashBlock(uintptr_t bar, unsigned address, int sleept)
{
  int ret;

  rorcWriteReg(bar, F_IFDSR, address);
  usleep(sleept);
  rorcWriteReg(bar, F_IFDSR, address);
  usleep(sleept);
  rorcWriteReg(bar, F_IFDSR, 0x03000020);
  usleep(sleept);
  rorcWriteReg(bar, F_IFDSR, 0x030000d0);
  usleep(sleept);
  ret = checkFlashStatus(bar, MAX_WAIT);

  return (ret);
}

int writeFlashWord(uintptr_t bar, unsigned address, int value, int sleept)
{
  int ret;

  rorcWriteReg(bar, F_IFDSR, address);
  usleep(sleept);
  rorcWriteReg(bar, F_IFDSR, 0x03000040);
  usleep(sleept);
  rorcWriteReg(bar, F_IFDSR, value);
  usleep(sleept);
  ret = checkFlashStatus(bar, MAX_WAIT);

  return (ret);
}

int readFlashWord(uintptr_t bar, unsigned address, char *data, int sleept)
{
  unsigned stat;

  rorcWriteReg(bar, F_IFDSR, address);
  usleep(sleept);
  rorcWriteReg(bar, F_IFDSR, 0x030000ff);
  usleep(sleept);
  stat = readFlashStatus(bar, sleept);
  *data = (stat & 0xFF00) >> 8;
  *(data+1) = stat & 0xFF;
  return (RORC_STATUS_OK);
}
