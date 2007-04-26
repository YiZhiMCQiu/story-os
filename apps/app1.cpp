//    This file is part of the Story OS
//    Copyright (C) 2007  Peter Zotov
//
//    Story OS is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    Story OS is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <system.h>
#include <string.h>
#include <ipc.h>
#include <stdio.h>

int main()
{
MessageType type = mtUnknown;
char* data = "hello!";
unsigned int task = 4;
printf("app1: sending type=%i data='%s' length=%i to task %i... ", type, data, strlen(data), task);
bool success = Message(type, task, data, strlen(data)).send();
if(success == 0)
 printf("success!\n");
else
 printf("error!\n");
*((int*)0) = 0;
printf("app1: exit\n");
}
