#pragma once

#define FULL_WRITE(fd, buf, count)                                             \
  do {                                                                         \
    ssize_t res;                                                               \
    if ((res = write(fd, buf, count)) < 0)                                     \
      perror("Write");                                                         \
  } while (0)