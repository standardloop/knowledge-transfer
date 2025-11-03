# Kind with Gateway API using `cloud-provider-kind`

## Background

*kind is a tool for running local Kubernetes clusters using Docker container “nodes”.
kind was primarily designed for testing Kubernetes itself, but may be used for local development or CI.*

Kind is part of the Kubernetes Special Interest Groups (SIGS)

Special Interest Groups in Kubernetes provide places for the community to focus development and discussion on a particular part of the project.

You can find the Kind source code here:
- https://github.com/kubernetes-sigs/kind

and the documentation here:
- https://kind.sigs.k8s.io/

## Prerequisites

### General Knowledge
- `docker`
- `brew` (or your preferred package manager)
- YAML
- `kubectl`

### Install and Setup

#### Docker
Have `docker` installed and have a docker engine running.

For this article, I will be using [Rancher Desktop](https://github.com/rancher-sandbox/rancher-desktop/).

You can install with Rancher Desktop with `brew` like this:

```sh
$ brew install --cask rancher
```

You can check your `docker` version with the following command:
```sh
$ docker --version
Docker version 28.1.1-rd, build 4d7f01e
```

Make sure your `docker` engine is running.
You can check if it is with the following command:
```sh
$ docker info > /dev/null 2>&1
$ echo $?
0  # if it isn't running you should see a 1
```

#### `kind`

I recommend installing using a package manager such as `brew` for macOS:

```sh
$ brew install kind
$ which kind
/opt/homebrew/bin/kind
$ kind --version
kind version 0.30.0
```

## Creating a cluster with a yaml config file

- https://kind.sigs.k8s.io/docs/user/configuration/

### Background

I will naming my cluster `slke-1` (standardloop kubernetes engine 1).

You can create clusters without config files, but I always recommend getting in the habit of using them for repeatable behavior.

---

There are two kinds of node types, `control-plane` and `worker`.

A single `control-plane` node is always required.

If you try to make one without, you will see an error message such as this.

```sh
ERROR: failed to create cluster: must have at least one control-plane node
```


### Config

Make sure to check the newest version, you can find the images here:
- https://hub.docker.com/r/kindest/node/

I will create a file named `kind-config.yaml` and place the following contents in it:

```yaml
---
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
name: slke-1
nodes:
  - role: control-plane
    image: kindest/node:v1.34.0
  - role: worker
    image: kindest/node:v1.34.0
```
Here we are creating two nodes, one for the control-plane and one worker.


```sh
$ kind create cluster --config=./kind-config.yaml
...
```

You can use a tool such as [kubectx](https://github.com/ahmetb/kubectx) to ensure your context is set.
```sh
$ kubectx
kind-slke-1
```
Notice how the name is prefixed with `kind-`

### See the nodes running


#### Use `docker ps` to see the nodes running
```sh
$ docker ps
CONTAINER ID   IMAGE                  COMMAND                  CREATED         STATUS         PORTS                       NAMES
4b59fab194e5   kindest/node:v1.34.0   "/usr/local/bin/entr…"   2 minutes ago   Up 2 minutes   127.0.0.1:57499->6443/tcp   slke-1-control-plane
bdf56ba1fc15   kindest/node:v1.34.0   "/usr/local/bin/entr…"   2 minutes ago   Up 2 minutes                               slke-1-worker
```

#### `kubectl get nodes`

```sh
$ kubectl get nodes
NAME                   STATUS   ROLES           AGE   VERSION
slke-1-control-plane   Ready    control-plane   15m   v1.34.0
slke-1-worker          Ready    <none>          15m   v1.34.0
```

If it is bothering you that `ROLES` is empty for the `worker` node, you can run:
```sh
$ kubectl label nodes slke-1-worker node-role.kubernetes.io/worker=""
node/slke-1-worker labeled
$ kubectl get nodes
NAME                   STATUS   ROLES           AGE   VERSION
slke-1-control-plane   Ready    control-plane   16m   v1.34.0
slke-1-worker          Ready    worker          16m   v1.34.0
```

### Updating the Cluster

You cannot update a cluster after creation, so you will need to delete it and then recreate it if you are changing any config.


### Setting up `cloud-provider-kind`

`cloud-provider-kind` allows you to use a [Kubernetes Gateway](https://kubernetes.io/docs/concepts/services-networking/gateway/#api-kind-gateway) resource and get an assignable IP address

#### Install

```sh
$ brew install cloud-provider-kind
$ which cloud-provider-kind
/opt/homebrew/bin/cloud-provider-kind
```

#### Run

I recommend opening a separate terminal session for running this command to easily view the logs.

According to the [README of the repository](https://github.com/kubernetes-sigs/cloud-provider-kind/blob/main/README.md), you will need to run with `sudo`
```sh
$ sudo cloud-provider-kind --gateway-channel standard
```

Notice the flag I am passing in.

You can watch the log information when a Kubernetes `Gateway` is created or deleted.


You can also view the docker container running:

```sh
$ docker ps
CONTAINER ID   IMAGE                      COMMAND                  CREATED          STATUS          PORTS                                                                                                                                     NAMES
a411bd911a94   envoyproxy/envoy:v1.33.2   "/docker-entrypoint.…"   4 seconds ago    Up 3 seconds    0.0.0.0:32774->80/tcp, [::]:32774->80/tcp, 0.0.0.0:32775->443/tcp, [::]:32775->443/tcp, 0.0.0.0:32776->10000/tcp, [::]:32776->10000/tcp   kindccm-f53170ec34f3
...
```

You can now see a [GatewayClass](https://kubernetes.io/docs/concepts/services-networking/gateway/#api-kind-gateway-class) was created for us in the cluster:

```sh
$ kubectl get GatewayClass
NAME                  CONTROLLER                            ACCEPTED   AGE
cloud-provider-kind   kind.sigs.k8s.io/gateway-controller   True       5m1s
```


### Deploying the Gateway Resource

I will create a file called `gateway.yaml` with the following contents:

```yaml
---
apiVersion: v1
kind: Namespace
metadata:
  name: gateway
---
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: gateway
  namespace: gateway
spec:
  gatewayClassName: cloud-provider-kind
  listeners:
    - name: http
      protocol: HTTP
      port: 80
      hostname: "kind.local"
      allowedRoutes:
        namespaces:
          from: All
```

Notice how the `gatewayClassName` references the `GatewayClass` that was created for us shown in the previous step.

Deploy with the following:

```sh
$ kubectl apply -f gateway.yaml
```

If you see this message:
```sh
error: resource mapping not found for name: "gateway" namespace: "gateway" from "gateway.yaml": no matches for kind "Gateway" in version "gateway.networking.k8s.io/v1"
ensure CRDs are installed first
```

Just run the apply command again, the `cloud-provider-kind` service may not have been running fully.

---
Easy way to print out your IP address:
```sh
$ kubectl get gateway/gateway -n gateway | awk '{print $3}' | awk NR==2
172.18.0.4
```

##### Use `curl` to hit the `Gateway`

```sh
$ curl http://172.18.0.4 -I
HTTP/1.1 404 Not Found
date: Mon, 03 Nov 2025 07:11:01 GMT
server: envoy
transfer-encoding: chunked
```

Awesome!


### Configure `/etc/hosts`

Using the IP address we got from:
```sh
$ kubectl get gateway/gateway -n gateway | awk '{print $3}' | awk NR==2
```

We can update `/etc/hosts` to have a nice looking URL.

```sh
$ sudo vim /etc/hosts
...
172.18.0.4      kind.local
```

Now you can go to http://kind.local

### Deploy a Sample App and use the Ingress Controller

I will be using the [hashicorp http-echo](https://github.com/hashicorp/http-echo) server has my example app.

Make sure to use the most up to date image, you can view the images [here](https://hub.docker.com/r/hashicorp/http-echo).

I will create a file called `app.yaml` and add the following manifests in it:

```yaml
---
apiVersion: v1
kind: Namespace
metadata:
  name: app
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: http-echo-deployment
  namespace: app
spec:
  replicas: 1
  selector:
    matchLabels:
      app: http-echo
  template:
    metadata:
      labels:
        app: http-echo
    spec:
      containers:
        - name: http-echo
          image: hashicorp/http-echo:1.0.0
          args: ["-text", "Hello from Kubernetes!", "-listen", ":8080"]
          ports:
            - containerPort: 8080
---
apiVersion: v1
kind: Service
metadata:
  name: http-echo-service
  namespace: app
spec:
  selector:
    app: http-echo
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: ClusterIP
---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: http-echo-route
  namespace: app
spec:
  parentRefs:
    - name: gateway
      namespace: gateway
  hostnames:
    - "kind.local"
  rules:
    - backendRefs:
        - name: http-echo-service
          port: 80
```

```sh
$ kubectl apply -f app.yaml
namespace/app created
deployment.apps/http-echo-deployment created
service/http-echo-service created
httproute.gateway.networking.k8s.io/http-echo-route created
```

Go to http://kind.local and see the message!


### Cleaning up

stop your `cloud-provider-kind` by Ctrl+C'ing the terminal or by looking the processes and stopping it.

And then delete your cluster:
```sh
$ kind delete cluster --name slke-1
```

## TLDR

Use my provided `Taskfile.yml` and run it all yourself easily!

- https://github.com/standardloop/knowledge-transfer/blob/main/02-kind-gateway-api/Taskfile.yml

```sh
$ task
```
