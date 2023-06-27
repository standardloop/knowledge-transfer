# Introduction to Local Kubernetes Using Kind

## Background

*kind is a tool for running local Kubernetes clusters using Docker container “nodes”.
kind was primarily designed for testing Kubernetes itself, but may be used for local development or CI.*

Kind is part of the Kubernetes Special Interest Groups (SIGS)

Special Interest Groups in Kubernetes provide places for the community to focus development and discussion on a particular part of the project.

You can find the Kind source code here:
https://github.com/kubernetes-sigs/kind

and the documentation here:
https://kind.sigs.k8s.io/

## Prerequisites

### General Knowledge
- Docker
- bash
- homebrew (or your preffered package manager)
- YAML

### Install and Setup

#### docker
Have `docker` installed and have a docker engine running.

For this article, I will be using [Docker Desktop](https://www.docker.com/products/docker-desktop/)

You can check your `docker` version with the following command:
```sh
$ docker --version
Docker version 20.10.8, build 3967b7d
```

Make sure your `docker` engine is running.
You can check if it is with the following command:
```sh
$ docker info > /dev/null 2>&1
$ echo $?
0  # if it isn't running you should see a 1
```

#### kubectl

*The Kubernetes command-line tool, kubectl, allows you to run commands against Kubernetes clusters. You can use kubectl to deploy applications, inspect and manage cluster resources, and view logs.*

You can install `kubectl` with the following command:
```sh
$ brew install kubernetes-cli
$ # verify install
$ which kubectl
/usr/local/bin/kubectl
$ kubectl --version
```

#### kind

I recommend install using a package manager such as `homebrew` for macOS

```sh
$ brew install kind
$ # verify install
$ which kind
/usr/local/bin/kind
$ kind --version
kind version 0.20.0
```

## Creating a simple cluster

```sh
$ kind create cluster
Creating cluster "kind" ...
 ✓ Ensuring node image (kindest/node:v1.27.3) 🖼 
 ✓ Preparing nodes 📦  
 ✓ Writing configuration 📜 
 ✓ Starting control-plane 🕹️ 
 ✓ Installing CNI 🔌 
```

### Output Explanation

#### Ensuring node image (kindest/node:v1.27.3)
Kind uses Docker Containers as Nodes and there is a Docker Image that is used for each Node. If you do not specify an image tag version, Kind will select the latest version for you!

A Node is a worker machine in Kubernetes and may be either a virtual or a physical machine, depending on the cluster. Each Node is managed by the control plane. A Node can have multiple pods, and the Kubernetes control plane automatically handles scheduling the pods across the Nodes in the cluster.