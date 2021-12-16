# for init RTC:
```c
twi_init_master();
rtc_init(); // return 0 if ok
```
# setting clock
```c
rtc_set_time_s(hrs, mins, sec);
//example:
rtc_set_time_s(12, 0, 50);
```
# read clock
```c
//declaration
struct tm* t = NULL;
//reading time to struct t
t = rtc_get_time();
//conversion to str
sprintf(buf, "%d:%d:%d\n", t->hour, t->min, t->sec);

```
# setting alarm
```c
rtc_set_alarm_s(hrs, mins, sec);
//example:
rtc_set_alarm_s(12, 1, 0);
```

# read alarm setting
```c
uint8_t hour, min, sec;
rtc_get_alarm_s(&hour, &min, &sec);
//OUTPUT as:
hour = 12
min = 1
sec = 0;
```

# pooling alarm
```c
rtc_check_alarm()
```