#!/bin/bash -x

# make sure to be in the same directory as this script
script_dir_abs=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${script_dir_abs}"

# Make sure only root can run our script
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

# install ssh key
mkdir -p /home/alarm/.ssh
echo "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDKrrR1SpRW+/3wqaEJZjTdoLSyaZt1g6DI2ByJKnk5eBQX6lx9Ds3YzWzwr3sf2uXH3d9fdXjEsIVY1yN/+eKD3E9e9An8krwfnYe8Aimxh6PMRCdCdW2OByPVrikjTjR4I7EDelSKmJz7M2JWLxY5cTBB2K4V4XaltvoMdxtFy2j7UCdwLwH36RTxDVMnFokGSlb79FlDP1qwPGmht9TdPfTIiqHakWnJbRj1IA0wWT+SeB8PwQOJZyn45+l03QmG4pS2U0LtuzUKjd1Et13oE5H6G/sC6pmuWYdYx4Wq7KDZI2O8DDRtuqmFdrb8kISWWfn7/8x6qly9I4pQ+o4J sahand@thinkpad" > /home/alarm/.ssh/authorized_keys

# install dependencies
pacman -S --noconfirm libbcm2835
