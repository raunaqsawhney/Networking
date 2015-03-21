#!/bin/bash          
echo 'Building ABP NAK...'
make abp_nak
echo 'Running ABP NAK...'
./abp_nak
echo 'Done ABP NAK.'
