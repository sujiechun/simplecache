// scache.cpp : Defines the entry point for the console application.
//
#include<string.h>
#include "stdafx.h"
#include"scache.h"

scache* cache_t;

int _tmain(int argc, _TCHAR* argv[])
{
	scache_config config;
	
	memset(&config, 0, sizeof(scache_config));
	config.ard = 0.5;
	cache_t = new_scache(&config, 0);

	caches_insert(cache_t, 0, "hello", 6);
	return 0;
}

