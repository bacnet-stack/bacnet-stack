VERSION = 1.1.0
DOCKER_IMAGE = aptfile-$(VERSION)

shellcheck:
ifeq ($(shell shellcheck > /dev/null 2>&1 ; echo $$?),127)
ifeq ($(shell uname),Darwin)
	brew install shellcheck
else
	sudo add-apt-repository 'deb http://archive.ubuntu.com/ubuntu trusty-backports main restricted universe multiverse'
	sudo apt-get update -qq && sudo apt-get install -qq -y shellcheck
endif
endif

lint: shellcheck
	shellcheck bin/aptfile

clean:
	rm -f *.deb

deb: clean
	sed -i -e 's/"VERSION"/$(VERSION)/' Dockerfile && rm Dockerfile-e
	docker build -t $(DOCKER_IMAGE) .
	bash -c 'ID=$$(docker run -i -a stdin $(DOCKER_IMAGE)) && docker cp $$ID:/data/aptfile_$(VERSION)_amd64.deb . && docker rm $$ID'
	git checkout -- Dockerfile

release:
	@git status | grep -q "working directory clean" || (echo "You have uncomitted changes" && exit 1)
	$(MAKE) deb

.PHONY: shellcheck lint clean deb release
