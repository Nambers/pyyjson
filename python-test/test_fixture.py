# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import pytest

import pyyjson

from util import read_fixture_bytes, read_fixture_str


class TestFixture:
    def test_apache(self):
        """
        loads(), dumps() apache.json
        """
        val = read_fixture_str("apache.json")
        read = pyyjson.loads(val)
        assert pyyjson.loads(pyyjson.dumps(read)) == read
        assert pyyjson.loads(pyyjson.dumps_to_bytes(read)) == read

    def test_canada(self):
        """
        loads(), dumps() canada.json
        """
        val = read_fixture_str("canada.json")
        read = pyyjson.loads(val)
        assert pyyjson.loads(pyyjson.dumps(read)) == read
        assert pyyjson.loads(pyyjson.dumps_to_bytes(read)) == read

    def test_citm_catalog(self):
        """
        loads(), dumps() ctm.json
        """
        val = read_fixture_str("ctm.json")
        read = pyyjson.loads(val)
        assert pyyjson.loads(pyyjson.dumps(read)) == read
        assert pyyjson.loads(pyyjson.dumps_to_bytes(read)) == read

    def test_github(self):
        """
        loads(), dumps() github.json
        """
        val = read_fixture_str("github.json")
        read = pyyjson.loads(val)
        assert pyyjson.loads(pyyjson.dumps(read)) == read
        assert pyyjson.loads(pyyjson.dumps_to_bytes(read)) == read

    def test_instruments(self):
        """
        loads(), dumps() instruments.json
        """
        val = read_fixture_str("instruments.json")
        read = pyyjson.loads(val)
        assert pyyjson.loads(pyyjson.dumps(read)) == read
        assert pyyjson.loads(pyyjson.dumps_to_bytes(read)) == read

    def test_mesh(self):
        """
        loads(), dumps() mesh.json
        """
        val = read_fixture_str("mesh.json")
        read = pyyjson.loads(val)
        assert pyyjson.loads(pyyjson.dumps(read)) == read
        assert pyyjson.loads(pyyjson.dumps_to_bytes(read)) == read

    def test_mqaq2016(self):
        """
        loads(), dumps() MotionsQuestionsAnswersQuestions2016.json
        """
        val = read_fixture_str("MotionsQuestionsAnswersQuestions2016.json")
        read = pyyjson.loads(val)
        assert pyyjson.loads(pyyjson.dumps(read)) == read
        assert pyyjson.loads(pyyjson.dumps_to_bytes(read)) == read

    def test_truenull(self):
        """
        loads(), dumps() truenull.json
        """
        val = read_fixture_str("truenull.json")
        read = pyyjson.loads(val)
        assert pyyjson.loads(pyyjson.dumps(read)) == read
        assert pyyjson.loads(pyyjson.dumps_to_bytes(read)) == read

    def test_tweet(self):
        """
        loads(), dumps() tweet.json
        """
        val = read_fixture_str("tweet.json")
        read = pyyjson.loads(val)
        assert pyyjson.loads(pyyjson.dumps(read)) == read
        assert pyyjson.loads(pyyjson.dumps_to_bytes(read)) == read

    def test_twitter(self):
        """
        loads(),dumps() twitter.json
        """
        val = read_fixture_str("twitter.json")
        read = pyyjson.loads(val)
        assert pyyjson.loads(pyyjson.dumps(read)) == read
        assert pyyjson.dumps(read).encode("utf-8") == pyyjson.dumps_to_bytes(read)

    def test_blns(self):
        """
        loads() blns.json JSONDecodeError

        https://github.com/minimaxir/big-list-of-naughty-strings
        """
        val = read_fixture_bytes("blns.txt")
        for line in val.split(b"\n"):
            if line and not line.startswith(b"#"):
                with pytest.raises(pyyjson.JSONDecodeError):
                    _ = pyyjson.loads(b'"' + val + b'"')
