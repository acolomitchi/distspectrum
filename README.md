# distspectrum


### Brief

1. **What** a Qt application which displays the distribution of distances between 
    random points inside a unit square. 
  * allows the generation of random point clusters - uniform distribution in a square or 
    normal distribution with customizable stddev and clip radius
  * allows the user to distort these point cluster by applying affine or projective 
    transforn from the tip-of-the-mouse pointer

2. **Why**: machine learning usually involves clustering. If you know how many clusters to seek,
     the problem is simpler than when you don't.
     I wanted a tool to explore what a computer would "see" if examining only the distribution
     of distances, perhaps such information may be used to in separatings clusters.
     That is to say, this is rather a toy for a human mind (mine) to explore a bit the problem
     
3. **How** to build:<ul>
  <li> *Prerequisites*<ol>
    <li>Qt 5.7.0 - may work with earlier, I haven't tried<br>
    Just make sure you have the QChart module installed</li>
    <li>[Eigen3.3](http://eigen.tuxfamily.org/index.php?title=Main_Page)+</li>
    <li>a C++ compiler able to do C++11 compilation which is also supported
    by Qt</li>
    <li>QtCreator 4.1 set to your liking (optional) - alternatively, just use `qmake` 
        after adjusting the `distspectrum.pro` file to build your desired debug/release 
        target (if you don't intend to develop, but just to use the toy, I strongly urge
        you to build the `release` version if you value you time).</li>
    </ol>
    </li>
    <li>Building:<ol>
      <li>adjust the `EIGEN_DIR` value in the `distspectrum.pro` file</li>
      <li>Build the project your usual way of building a Qt application</li>
      </ol>
    </li>
    </ul>
