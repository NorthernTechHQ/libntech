/*
  Copyright 2024 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#ifndef CFENGINE_IP_ADDRESS_H
#define CFENGINE_IP_ADDRESS_H

#include <buffer.h>

typedef struct IPAddress IPAddress;
typedef enum
{
    IP_ADDRESS_TYPE_IPV4,
    IP_ADDRESS_TYPE_IPV6
} IPAddressVersion;

/**
  @brief Creates a new IPAddress object from a string.
  @param source Buffer containing the string representation of the ip address.
  @return A fully formed IPAddress object or NULL if there was an error parsing the source.
  */
IPAddress *IPAddressNew(Buffer *source);

/**
  @brief Creates a new IPAddress object from a hex string (as in procfs).
  @param source Buffer containing the string representation of the ip address.
  @return A fully formed IPAddress object or NULL if there was an error parsing the source.
  */
IPAddress *IPAddressNewHex(Buffer *source);

/**
  @brief Destroys an IPAddress object.
  @param address IPAddress object to be destroyed.
  */
int IPAddressDestroy(IPAddress **address);
/**
  @brief Returns the type of address.
  @param address Address object.
  @return The type of address or -1 in case of error.
  */
int IPAddressType(IPAddress *address);
/**
  @brief Produces a fully usable IPV6 or IPV4 address string representation.
  @param address IPAddress object.
  @return A buffer containing an IPV4 or IPV6 address or NULL in case the given address was invalid.
  */
Buffer *IPAddressGetAddress(IPAddress *address);
/**
  @brief Recovers the appropriate port from the given address.
  @param address IPAddress object.
  @return A valid port for connections or -1 if it was not available.
  */
int IPAddressGetPort(IPAddress *address);
/**
  @brief Compares two IP addresses.
  @param a IP address of the first object.
  @param b IP address of the second object.
  @return 1 if both addresses are equal, 0 if they are not and -1 in case of error.
  */
int IPAddressIsEqual(IPAddress *a, IPAddress *b);
/**
  @brief Checks if a given string is a properly formed IP Address.
  @param source Buffer containing the string.
  @param address Optional parameter. If given and not NULL then an IPAdress structure will be created from the string.
  @return Returns true if the string is a valid IP Address and false if not. The address parameter is populated accordingly.
  */
bool IPAddressIsIPAddress(Buffer *source, IPAddress **address);
/**
  @brief Compares two IP addresses for sorting.
  @param a IP address of the first object.
  @param b IP address of the second object.
  @return true if a < b, false otherwise.
  */
bool IPAddressCompareLess(IPAddress *a, IPAddress *b);
/**
 * @brief Check if string is localhost IPv4-/IPv6 addresses.
 * 
 *        For IPv4, any address in the range 127.0.0.0/8 is local host. E.g.
 *        '127.0.0.1', '127.1.2.3', 127.0.0.0' or '127.255.255.255'.
 * 
 *        For IPv6, '::1' is the one and only local host address. IPv6
 *        addresses can be written in long- and short-form, as well as
 *        something in-between (e.g. 0:0:0:0:0:0:0:1 / ::1 / 0:0::0:1). All of
 *        these forms are accepted.
 * @param str The string to check.
 * @return True if string is localhost, else false.
 */
bool StringIsLocalHostIP(const char *str);

#endif // CFENGINE_IP_ADDRESS_H
